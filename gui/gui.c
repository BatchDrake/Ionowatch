/*
    This file is part of Ionowatch.

    (c) 2011 Gonzalo J. Carracedo <BatchDrake@gmail.com>
    
    Ionowatch is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ionowatch is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <ionowatch.h>

#include "gui.h"

PTR_LIST_EXTERN (struct ionogram_index, inventories);
PTR_LIST_EXTERN (struct station_info, stations);
PTR_LIST_EXTERN (struct datasource, global_datasource);

extern struct datasource           *selected_datasource;
extern struct datasource_magnitude *selected_magnitude;
extern struct painter              *selected_painter;
extern struct station_info         *selected_station;

extern float  selected_freq;
extern float  selected_height;

extern float  selected_sunspot;
extern time_t selected_time;

struct strlist recent_files;


GtkListStore *station_select_store;
GtkListStore *predictor_select_store;
GtkListStore *magnitude_select_store;
GtkTreeStore *repo_tree_store;
GtkListStore *display_style_store;
GtkTreeView *repo_tree_view;

GtkWidget *main_window;
GtkWidget *predictor_vbox;
GtkWidget *predictor_radio;
GtkWidget *predictor_select_combo;
GtkWidget *station_select_combo;
GtkWidget *magnitude_select_combo;
GtkWidget *actual_data_source_radio;
GtkWidget *numerical_data_text;
GtkWidget *frequency_spin;
GtkWidget *height_spin;
GtkWidget *file_menu;
GtkWidget *show_station_check;
GtkWidget *shade_check, *evaluate_all_check, *grid_check, *verbose_check;

GtkWidget *north_latitude_spin, *east_longitude_spin, *monthly_sunspot_spin, *hour_spin, *min_spin, *sec_spin, *date_calendar;

GtkWidget *display_style_combo;
GtkWidget *ionowatch_about_dialog;

GtkWidget *error_label;
GtkWidget *status_label;

struct station_info *
ionowatch_get_selected_station (void) /* XXX: This function is a very BAD idea. Update a pointer directly */
{
  GtkTreeIter iter;
  GValue value = {0,};
  gboolean result;
  struct station_info *station;
  
  PROTECT (result = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (station_select_combo), &iter));
  
  if (result == FALSE)
    return NULL; /* No station selected. Weird. */
    
  PROTECT (gtk_tree_model_get_value (GTK_TREE_MODEL (station_select_store), &iter, 1, &value));
  
  PROTECT (station = g_value_get_pointer (&value));
  
  PROTECT (g_value_unset (&value));
  
  return station;
}

void
ionowatch_set_last_error (const char *fmt, ...)
{
  va_list ap;
  char *error;
  
  va_start (ap, fmt);

  error = vstrbuild (fmt, ap);
  
  if (error_label == NULL)
    fprintf (stderr, "[set_last_error] %s\n", error);
  else
    PROTECT (gtk_label_set_text (GTK_LABEL (error_label), error));  
  
  free (error);
  
  va_end (ap);
}

void
ionowatch_set_status (const char *fmt, ...)
{
  va_list ap;
  char *status;
  
  va_start (ap, fmt);
  
  status = vstrbuild (fmt, ap);
  
  if (status_label == NULL)
    fprintf (stderr, "[status] %s\n", status);
  else
    PROTECT (gtk_label_set_text (GTK_LABEL (status_label), status));

  free (status);
  
  va_end (ap);
}

struct datasource *
ionowatch_get_selected_predictor (void)
{
  GtkTreeIter iter;
  GValue value = {0,};
  gboolean result;
  struct datasource *station;
  
  PROTECT (result = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (predictor_select_combo), &iter));
  
  if (result == FALSE)
    return NULL; /* No station selected. Weird. */
    
  PROTECT (gtk_tree_model_get_value (GTK_TREE_MODEL (predictor_select_store), &iter, 1, &value));
  
  PROTECT (station = g_value_get_pointer (&value));
  
  PROTECT (g_value_unset (&value));
  
  return station;
}

/* TODO: make this more wisely */
void
ionowatch_select_station (struct station_info *info)
{
  int i;
  
  for (i = 0; i < stations_count; i++)
    if (stations_list[i] == info)
    {
      gtk_combo_box_set_active (GTK_COMBO_BOX (station_select_combo), i + 1);
      return;
    }
    
  gtk_combo_box_set_active (GTK_COMBO_BOX (station_select_combo), 0);
}



void
ionowatch_append_recent_file (const char *file)
{
  GtkWidget *new;
  
  int i;
  
  for (i = 0; i < recent_files.strings_count; i++)
    if (recent_files.strings_list[i] != NULL)
      if (strcmp (recent_files.strings_list[i], file) == 0)
        return;
        
  strlist_append_string (&recent_files, file);
  
  new = gtk_menu_item_new_with_label (file);
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), new);
  gtk_widget_show (new);
  
  g_signal_connect (new, "activate", G_CALLBACK (recent_file_activate_cb), xstrdup (file));
  
}


void
ionowatch_set_numerical_data (const char *fmt, ...)
{
  va_list ap;
  char *msg;
  char *full;
  
  va_start (ap, fmt);
  
  msg = vstrbuild (fmt, ap);
  
  full = strbuild ("<b><big>Numerical data</big></b>\n\n"
                   "<b>Selected station: </b>%s\n"
                   "<b>Location: </b>%s (%s)\n"
                   "<b>Coordinates: </b>%lfº N, %lfº E\n\n%s",
                   selected_station->name,
                   selected_station->name_long, selected_station->country,
                   selected_station->lat, selected_station->lon,
                   msg);
                   
  free (msg);
  
  gtk_label_set_markup (GTK_LABEL (numerical_data_text), full);
  
  free (full);

  va_end (ap);
}

void
ionowatch_save_recent_file_list (void)
{
  char *list;
  int i;
  int count;
  
  FILE *fp;
  
  if ((list = locate_config_file ("recent.lst", CONFIG_WRITE | CONFIG_CREAT | CONFIG_LOCAL)) == NULL)
  {
    ERROR ("recent file list couldn't be saved (wrong permissions?)");
    return;
  }
  
  if ((fp = fopen (list, "wb")) == NULL)
  {
    ERROR ("couldn't write to %s: %s", list, strerror (errno));
    free (list);
    return;
  }
  
  free (list);
  
  count = recent_files.strings_count;
  
  if (count > RECENT_MAX_SAVE)
    i = count - RECENT_MAX_SAVE;
  else
    i = 0;
    
  for (; i < recent_files.strings_count; i++)
    fprintf (fp, "%s\n", recent_files.strings_list[i]);
    
  fclose (fp);
  
}

void
ionowatch_load_recent_file_list (void)
{
  char *list;
  char *line;
  
  FILE *fp;
  
  if ((list = locate_config_file ("recent.lst", CONFIG_READ | CONFIG_LOCAL)) == NULL)
    return;
  
  if ((fp = fopen (list, "rb")) == NULL)
  {
    ERROR ("couldn't load file list: %s", strerror (errno));
    free (list);
    return;
  }
  
  while ((line = fread_line (fp)) != NULL)
  {
    ionowatch_append_recent_file (line);
    free (line);
  }
  
  fclose (fp);
}

static void
current_datasource_to_numerical_data (void)
{
  int i, result;
  float value;
  char *msg, *tmp;

  msg = xstrdup ("Evaluation of this datasource:\n\n");
  
  for (i = 0; i < selected_datasource->magnitude_count; i++)
    if (selected_datasource->magnitude_list[i] != NULL)
    {
      result = datasource_eval_station
               (selected_datasource,
                selected_datasource->magnitude_list[i],
                selected_station,
                selected_sunspot,
                selected_freq,
                selected_height,
                selected_time, &value);
                
      tmp = strbuild ("%s<b>%s:</b> ",
        msg,
        selected_datasource->magnitude_list[i]->desc);
        
      free (msg);
      msg = tmp;
      
      switch (result)
      {
        case EVAL_CODE_DATA:
          tmp = strbuild ("%s%f\n", msg, value);
          break;
          
        case EVAL_CODE_HOLE:
          tmp = strbuild ("%s<i>(not present)</i>\n", msg);
          break;
          
        case EVAL_CODE_WAIT:
          tmp = strbuild ("%s<i>(data is being retrieved)</i>\n", msg);
          break;
          
        case EVAL_CODE_NODATA:
          tmp = strbuild ("%s<i>(error obtaining data)</i>\n", msg);
          break;
          
        default:
          tmp = strbuild ("%s<i>(unknown eval code %d)</i>\n", msg, result);
          break;
      }
      
      free (msg);
      msg = tmp;
    }
    
  ionowatch_set_numerical_data (msg);
  
  free (msg);
}

int
ionowatch_data_is_ready (void)
{
  return selected_painter != NULL &&
         selected_station != NULL &&
         selected_datasource != NULL &&
         selected_magnitude != NULL;
}


void
ionowatch_evaluate_all (void)
{
  gboolean result;
  
  if (ionowatch_data_is_ready ())
  {
    painter_set_latitude (selected_painter, selected_station->lat);
    painter_set_longitude (selected_painter, selected_station->lon);
    painter_set_sunspot (selected_painter, selected_sunspot);
    painter_set_freq (selected_painter, selected_freq);
    painter_set_height (selected_painter, selected_height);
    
    painter_set_time (selected_painter, selected_time);
    painter_set_station (selected_painter, selected_station);
    painter_set_datasource (selected_painter, selected_datasource);
    painter_set_magnitude (selected_painter, selected_magnitude);
    painter_update_model (selected_painter);
    
    PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (actual_data_source_radio)));
  
    current_datasource_to_numerical_data ();
  }
  else
  {
    
    gl_globe_notify_repaint ();
    
  }
    
  ionogram_invalidate ();
  mufgraph_invalidate ();
}

void
ionowatch_notify_datasource_list_event (void)
{
  int count = 0;
  
  if (predictor_radio == NULL)
    return;
    
  count = ionowatch_refresh_predictor_select_store ();
  
  gtk_widget_set_sensitive (predictor_radio, count > 0 ? TRUE : FALSE);
}


