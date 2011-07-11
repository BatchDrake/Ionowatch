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

PTR_LIST_EXTERN (struct ionogram_index, inventories);
extern GtkWidget *actual_data_source_radio;
extern GtkWidget *frequency_spin;

struct datasource           *selected_datasource;
struct datasource_magnitude *selected_magnitude;
struct painter              *selected_painter;
struct station_info         *selected_station;

float  selected_freq;
float  selected_height;
float  selected_sunspot;
time_t selected_time;

struct station_info          dummy_station = 
  {NULL, "User selected location", "None", "", 0.0, 0.0, 0};

/* TODO: define this as a notification function */
void
ionowatch_switch_station (struct station_info *station)
{
  struct file_tree_node *root;
  gboolean result;
  
  if (selected_station != station)
  {  
    selected_station = station;
    
    PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (actual_data_source_radio)));
    
    if (result == TRUE)
    {
      /* XXX: kill all pending threads */
      ionowatch_clear_file_tree (); /* XXX: we don't need a singleton here! */
      root = ionowatch_get_fake_root ();
      
      root->station = station;
      root->url = strbuild ("%s%s/individual/", inventories_list[0]->url, root->station->name);
      
      ionowatch_set_numerical_data ("<i>(no data obtained)</i>");
      
      if (station != &dummy_station)
      {
        ionowatch_set_fetching_filelist ();

        ionowatch_queue_file_list (root);
      }
    }
    
    ionowatch_evaluate_all ();
  }
}

void
ionowatch_switch_magnitude (struct datasource_magnitude *magnitude)
{
  selected_magnitude = magnitude;
  ionowatch_evaluate_all ();
}

void
ionowatch_parse_ionogram_file (const char *file)
{
  gboolean result;
  struct station_info *station;
  struct ionogram_filename fn;
  
  if (ionogram_parse_filename (file, &fn) != -1)
  {
    if ((station = station_lookup (fn.station)) == NULL)
    {
      ERROR ("station %s doesn't exists, how can this possibly happen?",
        fn.station);
        
      return;
    }
    
    ionowatch_append_recent_file (file);
  
    PROTECT (result = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (actual_data_source_radio)));
    
    ionowatch_switch_station (station);
    ionowatch_select_station (station);
  
    PROTECT (selected_freq = gtk_spin_button_get_value (GTK_SPIN_BUTTON (frequency_spin)));
    
    selected_time = fn.time;
    gl_globe_set_time (selected_time);
    
    ionowatch_set_status ("Fetching ionogram...");
    
    if (result != TRUE)
      PROTECT (gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (actual_data_source_radio), TRUE));
    else
      ionowatch_evaluate_all ();
  }
  else
    ERROR ("ionogram filename \"%s\" has wrong format", file);
}

/* Notification method */
void
ionowatch_switch_datasource (struct datasource *source)
{
  if (selected_datasource != source)
  {
    selected_datasource = source;
    
    ionowatch_set_status ("Switching to datasource %s...", source->desc);
    ionowatch_refresh_magnitude_store ();
    ionowatch_evaluate_all ();
    ionowatch_set_status ("Switched to datasource %s", source->desc);
  }
}

void
ionowatch_switch_painter (struct painter *painter)
{
  if (selected_painter != painter)
  {
    selected_painter = painter;
    ionowatch_evaluate_all ();
  }
}

#define safefprintf(fp, fmt, arg...) \
  if (fprintf (fp, fmt, ##arg) < 1) \
  {                                 \
   fclose (fp);                     \
   return -1;                       \
  }
  
int
export_matlab (const char *filename)
{
  FILE *fp;
  time_t now;
  struct tm tm;
  int i, result;
  float value;
  
  if (!ionowatch_data_is_ready ())
  {
    errno = EAGAIN; /* For example */
    return -1;
  }
  
  if ((fp = fopen (filename, "wb")) == NULL)
    return -1;
  
  time (&now);
  
  gmtime_r (&now, &tm);
  
  safefprintf (fp, "%% MATLAB file exported by Ionowatch on %s", ctime (&now));
  safefprintf (fp, "%% File was generated using datasource \"%s\"\n", selected_datasource->desc);
  safefprintf (fp, "%% Selected station is in %s (%s)\n", selected_station->name_long, selected_station->country);
  
  safefprintf (fp, "%% Selected parameters for this datafile:\n\n");
  safefprintf (fp, "selected_freq      = %.6lf;\n", selected_freq);
  safefprintf (fp, "selected_height    = %.6lf;\n", selected_height);
  safefprintf (fp, "selected_sunspot   = %.6lf;\n", selected_sunspot);
  safefprintf (fp, "selected_unix_time = %i;\n", selected_time);
  safefprintf (fp, "selected_date      = [%d %d %d %d %d %d %d];\n", 
    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
    tm.tm_yday + 1, tm.tm_hour, tm.tm_min, tm.tm_sec);
    
  safefprintf (fp, "\n");
  safefprintf (fp, "%% Evaluation of all magnitudes:\n\n");

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
                

      switch (result)
      {
        case EVAL_CODE_DATA:
          safefprintf (fp, "%% %s\n", selected_datasource->magnitude_list[i]->desc);
          safefprintf (fp, "mag_%s = %.8lf;\n\n", selected_datasource->magnitude_list[i]->name, value);
          break;
          
        case EVAL_CODE_HOLE:
          safefprintf (fp, "%% \"%s\" is not present with the selected configuration\n\n", 
            selected_datasource->magnitude_list[i]->desc);
          break;
          
        case EVAL_CODE_WAIT:
          safefprintf (fp, "%% \"%s\" was being retrieved during the exportation of this file\n\n", 
            selected_datasource->magnitude_list[i]->desc);
          break;
          
        case EVAL_CODE_NODATA:
          safefprintf (fp, "%% \"%s\" couldn't be provided by this datasource\n\n", 
            selected_datasource->magnitude_list[i]->desc);
          break;
          
        default:
          safefprintf (fp, "%% \"%s\" made datasource return an unknown status code\n\n", 
            selected_datasource->magnitude_list[i]->desc);
          break;
      }
    }
    
  fclose (fp);
  
  return 0;
}



