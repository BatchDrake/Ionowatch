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

#include <ionowatch.h>

#include "gui.h"

extern time_t selected_time;
extern struct station_info         *selected_station;
extern struct painter              *selected_painter;

extern float  selected_freq;
extern float  selected_height;
extern float  selected_sunspot;

extern GtkWidget   *actual_data_source_radio;
extern GtkWidget *main_window;
extern GtkTreeView *repo_tree_view;
extern GtkWidget   *predictor_vbox;

extern GtkWidget *display_style_combo;
extern GtkListStore *display_style_store;

extern GtkWidget *station_select_combo;
extern GtkWidget *magnitude_select_combo;

extern GtkWidget *north_latitude_spin;
extern GtkWidget *east_longitude_spin;
extern GtkWidget *monthly_sunspot_spin;
extern GtkWidget *hour_spin;
extern GtkWidget *min_spin; 
extern GtkWidget *sec_spin;
extern GtkWidget *date_calendar;

extern GtkWidget *shade_check;
extern GtkWidget *evaluate_all_check;
extern GtkWidget *grid_check;
extern GtkWidget *verbose_check;
extern GtkWidget *show_station_check;
extern GtkWidget *ionowatch_about_dialog;
extern GtkWidget *frequency_spin;
extern GtkWidget *height_spin;

extern GtkListStore *magnitude_select_store;
extern GtkTreeStore *repo_tree_store;

extern struct station_info dummy_station;

static void
update_selected_time (void)
{
  struct tm tm;
  
  gmtime_r (&selected_time, &tm);
    
  PROTECT (gtk_spin_button_set_value (GTK_SPIN_BUTTON (hour_spin), tm.tm_hour));
  PROTECT (gtk_spin_button_set_value (GTK_SPIN_BUTTON (min_spin), tm.tm_min));
  PROTECT (gtk_spin_button_set_value (GTK_SPIN_BUTTON (sec_spin), tm.tm_sec));
  
  PROTECT (gtk_calendar_select_month (GTK_CALENDAR (date_calendar), tm.tm_mon, tm.tm_year + 1900));
  PROTECT (gtk_calendar_select_day (GTK_CALENDAR (date_calendar), tm.tm_mday));
}

void
save_ionogram_cb (GtkWidget *widget, gpointer unused)
{
  GtkWidget *dialog;
  cairo_surface_t *surface;
  cairo_t *cr;
  char *filename;
  
  enter_callback_context ();
  
  dialog = gtk_file_chooser_dialog_new ("Save ionogram as PNG",
                                        GTK_WINDOW (main_window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                        NULL);
  
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    
    surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 
                                          800, 600);
                                          
    cr = cairo_create (surface);
    
    ionogram_paint_on_cairo (cr, 800, 600);
    
    if (cairo_surface_write_to_png (surface, filename) != CAIRO_STATUS_SUCCESS)
      ionowatch_generic_error ("Error saving file", "Save ionogram as PNG");
    
    cairo_destroy (cr);
    
    cairo_surface_destroy (surface);
    
    g_free (filename);
  }
  
  gtk_widget_destroy (dialog);
  
  enter_callback_context ();
}

void
save_muf_cb (GtkWidget *widget, gpointer unused)
{
  GtkWidget *dialog;
  cairo_surface_t *surface;
  cairo_t *cr;
  char *filename;
  
  enter_callback_context ();
  
  dialog = gtk_file_chooser_dialog_new ("Save MUF graph as PNG",
                                        GTK_WINDOW (main_window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                        NULL);
  
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    
    surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 
                                          800, 600);
                                          
    cr = cairo_create (surface);
    
    muf_paint_on_cairo (cr, 800, 600);
    
    if (cairo_surface_write_to_png (surface, filename) != CAIRO_STATUS_SUCCESS)
      ionowatch_generic_error ("Error saving file", "Save MUF as PNG");
    
    cairo_destroy (cr);
    
    cairo_surface_destroy (surface);
    
    g_free (filename);
  }
  
  gtk_widget_destroy (dialog);
  
  enter_callback_context ();
}

void
export_matlab_cb (GtkWidget *widget, gpointer unused)
{
  GtkWidget *dialog;
  char *filename;
  char *error;
  
  enter_callback_context ();
  
  dialog = gtk_file_chooser_dialog_new ("Export data as MATLAB...",
                                        GTK_WINDOW (main_window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                        NULL);
  
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    
    if (export_matlab (filename) == -1)
    {
      error = strbuild ("Couldn't export data as MATLAB: %s\n",
        strerror (errno));
        
      ionowatch_generic_error (error, "Export data as MATLAB");
      
      free (error);
    }
    
    
    g_free (filename);
  }
  
  gtk_widget_destroy (dialog);
  
  enter_callback_context ();
}

void
data_source_toggle_cb (GtkComboBox *widget, gpointer unused)
{
  gboolean result;
  extern struct datasource *scaled_datasource;
  
  enter_callback_context ();
  
  PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (actual_data_source_radio)));
  
  update_selected_time ();
  
  if (result == TRUE)
  {
    PROTECT (gtk_widget_set_sensitive (GTK_WIDGET (repo_tree_view), TRUE));
    PROTECT (gtk_widget_set_sensitive (predictor_vbox, FALSE));
    
    ionowatch_switch_datasource (scaled_datasource);
  }
  else
  {
    PROTECT (gtk_widget_set_sensitive (GTK_WIDGET (repo_tree_view), FALSE));
    PROTECT (gtk_widget_set_sensitive (predictor_vbox, TRUE));
    
    ionowatch_switch_datasource (ionowatch_get_selected_predictor ());
  }
  
  leave_callback_context ();
}

void
show_credits_cb (GtkWidget *widget, gpointer unused)
{
  enter_callback_context ();
  
 // PROTECT (gtk_widget_show (ionowatch_about_dialog));
  PROTECT (gtk_widget_set_visible (ionowatch_about_dialog, TRUE));
  
  leave_callback_context ();
}

void
about_dialog_close_cb (GtkWidget *widget, gpointer unused)
{
  enter_callback_context ();
  
 // PROTECT (gtk_widget_hide (ionowatch_about_dialog));
  PROTECT (gtk_widget_set_visible (ionowatch_about_dialog, FALSE));
  leave_callback_context ();
}

void
change_station_cb (GtkComboBox *widget, gpointer unused)
{
  struct station_info *selected;
  
  enter_callback_context ();
  
  selected = ionowatch_get_selected_station ();
  
  ionowatch_switch_station (selected);
  
  PROTECT (gtk_spin_button_set_value (GTK_SPIN_BUTTON (north_latitude_spin), selected_station->lat));
  PROTECT (gtk_spin_button_set_value (GTK_SPIN_BUTTON (east_longitude_spin), selected_station->lon));
  
  if (selected == &dummy_station)
  {
    PROTECT (gtk_widget_set_sensitive (GTK_WIDGET (north_latitude_spin), TRUE));
    PROTECT (gtk_widget_set_sensitive (GTK_WIDGET (east_longitude_spin), TRUE));
  }
  else
  {
    PROTECT (gtk_widget_set_sensitive (GTK_WIDGET (north_latitude_spin), FALSE));
    PROTECT (gtk_widget_set_sensitive (GTK_WIDGET (east_longitude_spin), FALSE));
  }
  gl_globe_notify_repaint ();
    
  leave_callback_context ();
}


void
predictor_changed_cb (GtkComboBox *widget, gpointer unused)
{
  gboolean result;
  
  enter_callback_context ();
  
  PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (actual_data_source_radio)));
  
  if (result == FALSE)
    ionowatch_switch_datasource (ionowatch_get_selected_predictor ());
  
  gl_globe_notify_repaint ();
    
  leave_callback_context ();
}

void
change_magnitude_cb (GtkComboBox *widget, gpointer unused)
{
  GtkTreeIter iter;
  GValue value = {0,};
  gboolean result;
  struct datasource_magnitude *magnitude;
  
  enter_callback_context ();
  
  PROTECT (result = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (magnitude_select_combo), &iter));
  
  if (result == TRUE)
  { 
    PROTECT (gtk_tree_model_get_value (GTK_TREE_MODEL (magnitude_select_store), &iter, 1, &value));
    
    PROTECT (magnitude = g_value_get_pointer (&value));
    
    PROTECT (g_value_unset (&value));
    
    ionowatch_switch_magnitude (magnitude);
  }
  
  leave_callback_context ();
}

void
display_style_change_cb (GtkComboBox *widget, gpointer unused)
{
  GtkTreeIter iter;
  GValue value = {0,};
  gboolean result;
  struct painter *painter;
  
  enter_callback_context ();
  
  PROTECT (result = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (display_style_combo), &iter));
  
  if (result == TRUE)
  { 
    PROTECT (gtk_tree_model_get_value (GTK_TREE_MODEL (display_style_store), &iter, 1, &value));
    
    PROTECT (painter = g_value_get_pointer (&value));
    
    PROTECT (g_value_unset (&value));
    
    ionowatch_switch_painter (painter);
  }
  
  leave_callback_context ();
}

void
window_close_cb (GtkWidget *widget, gboolean unused)
{
  enter_callback_context ();
  
  ionowatch_save_recent_file_list ();
  
  gtk_main_quit ();
  
  leave_callback_context ();
}

void
repo_tree_row_activated_cb (GtkTreeView *tree_view,
                            GtkTreePath *path,
                            GtkTreeViewColumn *column,
                            gpointer           user_data) 
{
  GtkTreeIter iter;
  struct file_tree_node *node;
  gboolean result;
  
  enter_callback_context ();

  
  PROTECT (result = gtk_tree_model_get_iter (GTK_TREE_MODEL (repo_tree_store), &iter, path));
  
  if (result)
  {
    /* TODO: use constants to refer columns */
    PROTECT (gtk_tree_model_get (GTK_TREE_MODEL (repo_tree_store), &iter, 5, &node, -1));
    
    if (node != NULL)
    {
      if (node->station != selected_station) /* This happens when we change station while in prediction mode */      
      {
        ionowatch_switch_station (node->station);
        ionowatch_select_station (node->station);
      }
      else if (node->type != FILE_TREE_NODE_TYPE_FILE)
      {
        if (!node->updated)
          ionowatch_queue_file_list (node);
      }
      else
        ionowatch_parse_ionogram_file (node->name);
      
    }
  }
  
  leave_callback_context ();
}

void
time_wrapped_cb (GtkWidget *widget, gpointer unused)
{
  gdouble value;
  int units;
  
  enter_callback_context ();
  
  PROTECT (value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget)));
  
  if (widget == sec_spin)
    units = 60;
  else if (widget == min_spin)
    units = 3600;
  else if (widget == hour_spin)
    units = 86400;
    
  if (value == 0.0)
    selected_time += units;
  else
    selected_time -= units;
    
  update_selected_time ();
  
  leave_callback_context ();
}

gboolean 
input_data_changed_cb (GtkRange *range, GtkScrollType scroll, gdouble value,
               gpointer user_data)
{
  
  
  struct tm tm;
  int h, m, s;
  gboolean result;
  
  guint year, month, day;
  
  enter_callback_context ();
  
  PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (actual_data_source_radio)));
  PROTECT (selected_freq = gtk_spin_button_get_value (GTK_SPIN_BUTTON (frequency_spin)));
  PROTECT (selected_height = gtk_spin_button_get_value (GTK_SPIN_BUTTON (height_spin)));
  
  if (result == FALSE)
  {
    PROTECT (dummy_station.lat = gtk_spin_button_get_value (GTK_SPIN_BUTTON (north_latitude_spin)));
    PROTECT (dummy_station.lon = gtk_spin_button_get_value (GTK_SPIN_BUTTON (east_longitude_spin)));
    
    
    PROTECT (selected_sunspot = gtk_spin_button_get_value (GTK_SPIN_BUTTON (monthly_sunspot_spin)));

    PROTECT (h = gtk_spin_button_get_value (GTK_SPIN_BUTTON (hour_spin)));
    PROTECT (m = gtk_spin_button_get_value (GTK_SPIN_BUTTON (min_spin)));
    PROTECT (s = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sec_spin)));

    PROTECT (gtk_calendar_get_date (GTK_CALENDAR (date_calendar), &year, &month, &day));
    
    tm.tm_sec = s;
    tm.tm_min = m;
    tm.tm_hour = h;
    tm.tm_mon = month;
    tm.tm_year = year - 1900;
    tm.tm_mday = day;
    tm.tm_isdst = -1;
    
    selected_time = mktime (&tm);
    
    gl_globe_set_time (selected_time);
  }
  
  ionowatch_evaluate_all ();
  
  leave_callback_context ();
  
  return FALSE;
}

void
display_options_toggled_cb (GtkComboBox *widget, gpointer unused)
{
  gboolean result;
  
  enter_callback_context ();
  
  PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (grid_check)));
  
  gl_globe_set_draw_grid (result);
  
  if (selected_painter != NULL)
  {
    PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (evaluate_all_check)));
    
    if (result)
      painter_set_options (selected_painter, PAINTER_OPTION_MASK_EVALUATE_ALL);
    else
      painter_unset_options (selected_painter, PAINTER_OPTION_MASK_EVALUATE_ALL);
      
    PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (verbose_check)));
    
    if (result)
      painter_set_options (selected_painter, PAINTER_OPTION_MASK_VERBOSE);
    else
      painter_unset_options (selected_painter, PAINTER_OPTION_MASK_VERBOSE);      
      
    PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_station_check)));
      
    gl_globe_set_draw_station (result);
    
    PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (shade_check)));
      
    gl_globe_set_shade (result);
  }
  
  if (GTK_WIDGET (widget) == GTK_WIDGET (evaluate_all_check))
    ionowatch_evaluate_all ();
  else
    gl_globe_notify_repaint ();
  
  leave_callback_context ();
}

void
recent_file_activate_cb (GtkWidget *widget, gpointer data)
{
  enter_callback_context ();
  
  ionowatch_parse_ionogram_file ((char *) data);
  
  leave_callback_context ();
}

