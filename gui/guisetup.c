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

PTR_LIST_EXTERN (struct datasource_magnitude, global_magnitude);
PTR_LIST_EXTERN (struct datasource, global_datasource);
PTR_LIST_EXTERN (struct station_info, stations);
PTR_LIST_EXTERN (struct painter, painter);

extern struct strlist recent_files;
extern struct datasource *scaled_datasource;
extern struct datasource           *selected_datasource;

static GdkColor black;

extern GtkListStore *station_select_store;
extern GtkWidget *station_select_combo;
extern GtkTreeStore *repo_tree_store;
extern GtkTreeView *repo_tree_view;
extern GtkWidget *actual_data_source_radio;
extern GtkWidget *predictor_vbox;
extern GtkListStore *magnitude_select_store;
extern GtkWidget *magnitude_select_combo;
extern GtkWidget *numerical_data_text;
extern GtkWidget *frequency_spin;
extern GtkWidget *height_spin;

extern GtkWidget *main_window;
extern GtkWidget *show_station_check;
extern GtkWidget *predictor_radio;
extern GtkWidget *shade_check, *evaluate_all_check, *grid_check, *verbose_check;
extern GtkWidget *north_latitude_spin, *east_longitude_spin, *monthly_sunspot_spin, *hour_spin, *min_spin, *sec_spin, *date_calendar;
extern GtkWidget *display_style_combo;
extern GtkListStore *display_style_store;
extern GtkWidget *file_menu;
extern GtkListStore *predictor_select_store;
extern GtkWidget *predictor_select_combo;
extern GtkWidget *ionogram_drawing_area;
extern GtkWidget *ionowatch_about_dialog;
extern GtkWidget *mufgraph_drawing_area;

extern GtkWidget *error_label;
extern GtkWidget *status_label;

extern time_t selected_time;

extern struct station_info dummy_station;

void
ionowatch_refresh_station_select_store (void)
{
  int i;
  char *station_string;
  GtkTreeIter iter;
  
  station_select_store = GTK_LIST_STORE (ionowatch_fetch_object ("StationSelectStore"));
  
  PROTECT (gtk_list_store_clear (station_select_store));

  PROTECT (gtk_list_store_append (station_select_store, &iter));
  PROTECT (gtk_list_store_set (station_select_store, &iter, 
    0, "User selected location", 1, &dummy_station, -1));
    
  for (i = 0; i < stations_count; i++)
    if (stations_list[i] != NULL)
    {
      station_string = strbuild ("%s (%s, %s)", 
        stations_list[i]->name, 
        stations_list[i]->name_long, 
        stations_list[i]->country);
        
      PROTECT (gtk_list_store_append (station_select_store, &iter));
      PROTECT (gtk_list_store_set (station_select_store, &iter, 
        0, station_string, 1, stations_list[i], -1));
      
      free (station_string);
    }
}

void
ionowatch_refresh_display_style_store (void)
{
  int i;
  GtkTreeIter iter;
  
  PROTECT (gtk_list_store_clear (display_style_store));

  for (i = 0; i < painter_count; i++)
    if (painter_list[i] != NULL)
    {
      PROTECT (gtk_list_store_append (display_style_store, &iter));
      PROTECT (gtk_list_store_set (display_style_store, &iter, 
        0, painter_list[i]->desc, 1, painter_list[i], -1));
        
    }
    
}

void
ionowatch_refresh_magnitude_store (void)
{
  int i;
  GtkTreeIter iter;
  
  PROTECT (gtk_list_store_clear (magnitude_select_store));

  for (i = 0; i < selected_datasource->magnitude_count; i++)
    if (selected_datasource->magnitude_list[i] != NULL)
    {
      PROTECT (gtk_list_store_append (magnitude_select_store, &iter));
      PROTECT (gtk_list_store_set (magnitude_select_store, &iter, 
        0, selected_datasource->magnitude_list[i]->desc, 1, selected_datasource->magnitude_list[i], -1));
    }
    
  PROTECT (gtk_combo_box_set_active (GTK_COMBO_BOX (magnitude_select_combo), 0));
}

int
ionowatch_refresh_predictor_select_store (void)
{
  int i, count;
  GtkTreeIter iter;
  
  PROTECT (gtk_list_store_clear (predictor_select_store));

  count = 0;
  for (i = 0; i < global_datasource_count; i++)
    if (global_datasource_list[i] != NULL && global_datasource_list[i] != scaled_datasource)
    {
      PROTECT (gtk_list_store_append (predictor_select_store, &iter));
      PROTECT (gtk_list_store_set (predictor_select_store, &iter, 
        0, global_datasource_list[i]->desc, 1, global_datasource_list[i], -1));
        
      count++;
        
    }
    
  if (count > 0)
    PROTECT (gtk_combo_box_set_active (GTK_COMBO_BOX (predictor_select_combo), 0));
    
  return count;
}

static void
iter_set_pulse (GtkTreeStore *store, GtkTreeIter *iter, guint pulse)
{
  gboolean state;
  
  gtk_tree_model_get (GTK_TREE_MODEL (store), iter, 3, &state, -1);
  
  if (state)
    gtk_tree_store_set (store, iter, 4, pulse, -1);
}

/* TODO: disable this timeout if there are no pending files */
static void
spin_pending_files_recursive (GtkTreeStore *store, GtkTreeIter *this, guint pulse)
{
  GtkTreeIter next;
  GtkTreeIter child;
   
   
  next = *this;
   
  do
  {
    iter_set_pulse (store, &next, pulse);
    
    if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, &next))
      spin_pending_files_recursive (store, &child, pulse);

  }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &next));
}

static gboolean
spin_pending_files (gpointer data)
{
  GtkTreeIter iter;
  
  static guint pulse;
  
  pulse++;
  
  if (pulse == G_MAXUINT)
    pulse = 0;
    
  if (gtk_tree_model_get_iter_first (data, &iter))
    spin_pending_files_recursive (data, &iter, pulse);
    
  return TRUE;
}

void
ionowatch_setup_tree_view (void)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  
  gint id;
  
  PROTECT (renderer = gtk_cell_renderer_spinner_new ());
  
  PROTECT (gtk_tree_view_insert_column_with_attributes (repo_tree_view,
                                               -1, "State", 
                                               renderer, "pulse", 4,
                                               "active", 3,
                                               NULL));
   
   
  PROTECT (renderer = gtk_cell_renderer_text_new ());
  
  id = gtk_tree_view_insert_column_with_attributes (repo_tree_view,
                                               -1, "Data file", 
                                               renderer, "text", 0,
                                               NULL);
  
  col = gtk_tree_view_get_column (repo_tree_view, id - 1);
  
  gtk_tree_view_set_expander_column (repo_tree_view, col);
  
  renderer = gtk_cell_renderer_text_new ();
  
  gtk_tree_view_insert_column_with_attributes (repo_tree_view,
                                               -1, "Date", 
                                               renderer, "text", 1,
                                               NULL);
  
  renderer = gtk_cell_renderer_text_new ();
  
  gtk_tree_view_insert_column_with_attributes (repo_tree_view,
                                               -1, "URL", 
                                               renderer, "text", 2,
                                               NULL);
                                               
   PROTECT (g_timeout_add (80, spin_pending_files, repo_tree_store));
  
}

/* TODO: we really need this?? */
void
ionowatch_refresh_models (void)
{
  ionowatch_refresh_station_select_store ();
  ionowatch_refresh_display_style_store ();
}

void
ionowatch_setup_datasource_selector (void)
{
  GtkWidget *wid;
  
  PROTECT (wid = ionowatch_fetch_widget ("ActualDataRadio"));
  
  PROTECT (gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE));
}


void
ionowatch_init_models (void)
{
  GtkCellRenderer *rend;
  
  time (&selected_time);
    
  main_window = ionowatch_fetch_widget ("IonowatchMainWindow");
  
  repo_tree_store = GTK_TREE_STORE (ionowatch_fetch_object ("RepoTreeStore"));
  repo_tree_view = GTK_TREE_VIEW (ionowatch_fetch_widget ("RepoTreeView"));
  
  magnitude_select_store = GTK_LIST_STORE (ionowatch_fetch_object ("MagnitudeStore"));
  magnitude_select_combo = ionowatch_fetch_widget ("MagnitudeCombo");
  
  predictor_select_store = GTK_LIST_STORE (ionowatch_fetch_object ("PredictorStore"));
  predictor_select_combo = ionowatch_fetch_widget ("PredictorCombo");
  
  actual_data_source_radio = ionowatch_fetch_widget ("ActualDataRadio");
  predictor_vbox = ionowatch_fetch_widget ("PredictorVBox");
  station_select_combo = ionowatch_fetch_widget ("StationSelect");
  numerical_data_text = ionowatch_fetch_widget ("NumericalData");
  frequency_spin = ionowatch_fetch_widget ("FrequencySelectSpin");
  height_spin = ionowatch_fetch_widget ("HeightSelectSpin");
  
  predictor_radio = ionowatch_fetch_widget ("PredictedDataRadio");
  
  north_latitude_spin = ionowatch_fetch_widget ("LatSpinButton");
  east_longitude_spin = ionowatch_fetch_widget ("LonSpinButton");
  monthly_sunspot_spin = ionowatch_fetch_widget ("SunspotCountButton");
  hour_spin = ionowatch_fetch_widget ("HourSpinButton");
  min_spin = ionowatch_fetch_widget ("MinSpinButton");
  sec_spin = ionowatch_fetch_widget ("SecSpinButton");
  date_calendar = ionowatch_fetch_widget ("DateCalendar");
  ionogram_drawing_area = ionowatch_fetch_widget ("IonogramDrawingArea");
  mufgraph_drawing_area = ionowatch_fetch_widget ("MUFGraphDrawingArea");
  
  ionowatch_about_dialog = ionowatch_fetch_widget ("IonowatchAboutDialog");
  
  gtk_widget_modify_bg (
    GTK_WIDGET (ionogram_drawing_area), GTK_STATE_NORMAL, &black);
  
  gtk_widget_modify_bg (
    GTK_WIDGET (mufgraph_drawing_area), GTK_STATE_NORMAL, &black);
  
  shade_check = ionowatch_fetch_widget ("ShadeCheck");
  evaluate_all_check = ionowatch_fetch_widget ("EvaluateAllCheck");
  grid_check = ionowatch_fetch_widget ("DrawGridCheck");
  show_station_check = ionowatch_fetch_widget ("ShowStationCheck");
  
  error_label = ionowatch_fetch_widget ("ErrorLabel");
  status_label = ionowatch_fetch_widget ("StatusLabel");
  
  verbose_check = ionowatch_fetch_widget ("VerboseCheck");
  file_menu = ionowatch_fetch_widget ("FileMenu");
  
  display_style_combo = ionowatch_fetch_widget ("DisplayStyleCombo");
  display_style_store = GTK_LIST_STORE (ionowatch_fetch_object ("DisplayStyleStore"));
  
  init_default_painter ();
  
  ionowatch_setup_tree_view ();
  ionowatch_setup_datasource_selector ();
  
  PROTECT (rend = gtk_cell_renderer_text_new ());
 
  PROTECT (gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (station_select_combo), rend, TRUE));
	PROTECT (gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (station_select_combo), rend, "text", 0));

  PROTECT (rend = gtk_cell_renderer_text_new ());
 
  PROTECT (gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (magnitude_select_combo), rend, TRUE));
	PROTECT (gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (magnitude_select_combo), rend, "text", 0));

  PROTECT (rend = gtk_cell_renderer_text_new ());
 
  PROTECT (gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (display_style_combo), rend, TRUE));
	PROTECT (gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (display_style_combo), rend, "text", 0));


  PROTECT (rend = gtk_cell_renderer_text_new ());
 
  PROTECT (gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (predictor_select_combo), rend, TRUE));
	PROTECT (gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (predictor_select_combo), rend, "text", 0));

	ionowatch_refresh_models ();
  
  PROTECT (gtk_combo_box_set_active (GTK_COMBO_BOX (station_select_combo), 1));
  
  if (painter_count != 0)
    PROTECT (gtk_combo_box_set_active (GTK_COMBO_BOX (display_style_combo), 0));
    
  ionowatch_load_recent_file_list ();

}

