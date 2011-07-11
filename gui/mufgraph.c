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

#define LINE(x) (MUFGRAPH_FONT_SIZE * x)

GtkWidget *mufgraph_drawing_area;

extern time_t selected_time;
extern struct datasource *scaled_datasource;
extern struct datasource *selected_datasource;
extern struct datasource_magnitude *selected_magnitude;
extern struct station_info         *selected_station;

extern struct datasource_magnitude *virtual_f2_height;
extern struct datasource_magnitude *virtual_f1_height;
extern struct datasource_magnitude *virtual_e_height;
extern struct datasource_magnitude *virtual_es_height;
extern struct datasource_magnitude *virtual_ea_height;
extern struct datasource_magnitude *plasma_frequency;
extern struct datasource_magnitude *plasma_peak_height;

extern float  selected_lat;
extern float  selected_lon;
extern float  selected_freq;
extern float  selected_height;
extern float  selected_sunspot;
extern time_t selected_time;

void
mufgraph_printf (cairo_t *cr, int x, int y, const char *fmt, ...)
{
  va_list ap;
  char *text;
  cairo_text_extents_t te;
  va_start (ap, fmt);
  
  text = vstrbuild (fmt, ap);
  
  
  cairo_select_font_face (cr, MUFGRAPH_FONT_FAMILY,
    CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    
  cairo_set_font_size (cr, MUFGRAPH_FONT_SIZE);
  cairo_text_extents (cr, text, &te);
  cairo_move_to (cr, x - te.x_bearing,
    y + MUFGRAPH_FONT_SIZE / 2 - te.height / 2 - te.y_bearing);
    
  cairo_show_text (cr, text);
        
  va_end (ap);
}

void
mufgraph_draw_grid (cairo_t *cr, int x, int y, int width, int height)
{
  static const double dashed[] = {1.0, 1.0};
  
  int i;
  
  cairo_move_to (cr, x, y);
  cairo_line_to (cr, x + width - 1, y);
  
  cairo_move_to (cr, x, y);
  cairo_line_to (cr, x, y + height - 1);
  
  cairo_move_to (cr, x + width - 1, y + height - 1);
  cairo_line_to (cr, x + width - 1, y);
  
  cairo_move_to (cr, x + width - 1, y + height - 1);
  cairo_line_to (cr, x, y + height - 1);
  
  cairo_stroke (cr);
  
  cairo_set_dash (cr, dashed, 2, 1);
  
  for (i = 0; i <= MUFGRAPH_GRID_TIME_STEPS; i++)
  {
    cairo_move_to (cr, x + (float) i / (float) MUFGRAPH_GRID_TIME_STEPS * (width - 1), y);
    cairo_line_to (cr, x + (float) i / (float) MUFGRAPH_GRID_TIME_STEPS * (width - 1), y + height - 1);
    
    if (! (i & 1))
      mufgraph_printf (cr, x + (float) i / (float) MUFGRAPH_GRID_TIME_STEPS * (width - 1) - LINE (1), 
        LINE (1) + y + height - 1, "%.1f", i / (float) MUFGRAPH_GRID_TIME_STEPS * 24.0);
  }
  
  for (i = 0; i <= MUFGRAPH_GRID_FREQ_STEPS; i++)
  {
    cairo_move_to (cr, x, y + (float) i / (float) MUFGRAPH_GRID_FREQ_STEPS * (height - 1));
    cairo_line_to (cr, x + width - 1, y + (float) i / (float) MUFGRAPH_GRID_FREQ_STEPS * (height - 1));
    
    mufgraph_printf (cr, x - LINE (4), 
                         y + (float) i / (float) MUFGRAPH_GRID_FREQ_STEPS * (height - 1) - LINE (0.5),
                         "+ %3.0f", MUFGRAPH_FREQ_MAX - i / (float) MUFGRAPH_GRID_FREQ_STEPS * MUFGRAPH_FREQ_MAX);
  }
  
  cairo_stroke (cr);
  
  mufgraph_printf (cr, x + width / 2 - LINE (7.5), y + height - 1 + LINE (2),
    "Hour of the day"); 
    
  mufgraph_printf (cr, x - LINE (4), y - LINE (1.5), "MUF (MHz)"); 
}

void
mufgraph_draw_mufs (cairo_t *cr, int x, int y, int width, int height)
{
  struct datasource_magnitude *this_magnitude;
  static const double dashed[] = {1.0};
  static float distances[] = {500.0, 1000.0, 2000.0, 3500.0};
  float freq;
  time_t utc_time;
  
  int i;
  int j;
  
  int last_state, this_state;
  float plasma_height, fc;
  float obf;
  float alpha;
  
  
  cairo_set_dash (cr, dashed, 0, 1);
  
  for (j = 0; j < 4; j++)
  {
    last_state = EVAL_CODE_HOLE;
    
    utc_time = (selected_time / 86400) * 86400;

    cairo_set_source_rgb (cr, 1.0, 0.2, 1.0);
    
    for (i = 0; i < MUFGRAPH_SAMPLES; i++)
    { 
      this_state = datasource_eval_station (
        selected_datasource, plasma_peak_height, selected_station, selected_sunspot, 0.0, 0.0, utc_time + i / (float) MUFGRAPH_SAMPLES * 86400.0, &plasma_height);
        
      if (this_state == EVAL_CODE_DATA)
        this_state = datasource_eval_station (
            selected_datasource, plasma_frequency, selected_station, selected_sunspot, 0.0, plasma_height, utc_time + i / (float) MUFGRAPH_SAMPLES * 86400.0, &fc);
            
      if (this_state == EVAL_CODE_DATA)
      {
        alpha = distances[j] / (2.0 * EARTH_RADIUS);
        
        obf = (plasma_height / EARTH_RADIUS + 1.0 - cos (alpha)) /
              sqrt (pow (plasma_height / EARTH_RADIUS + 1.0 - cos (alpha), 2.0) + pow (sin (alpha), 2.0));
              
        freq = fc / obf;
        
        if (last_state == EVAL_CODE_DATA)
        {
          cairo_line_to (cr, x + i * (width - 1) / (float) (MUFGRAPH_SAMPLES - 1), y + height - 1 - freq / MUFGRAPH_FREQ_MAX * (height - 1));

          cairo_stroke (cr);
        }
          
        cairo_move_to (cr, x + i  * (width - 1) / (float) (MUFGRAPH_SAMPLES - 1), y + height - 1 - freq / MUFGRAPH_FREQ_MAX * (height - 1));
      }
      
      last_state = this_state;
    }
    
    cairo_set_source_rgb (cr, 0.2, 1.0, 1.0);
    mufgraph_printf (cr, x + i  * (width - 1) / (float) MUFGRAPH_SAMPLES - LINE(2), y + height - 1 - freq / MUFGRAPH_FREQ_MAX * (height - 1) - LINE (0.5),
        "%lg km", distances[j]);
        
    if (last_state = EVAL_CODE_DATA)
    {
      cairo_line_to (cr, x + i  * (width - 1) / (float) MUFGRAPH_SAMPLES, y + height - 1 - freq / MUFGRAPH_FREQ_MAX * (height - 1));
      cairo_stroke (cr);
    }
  }
  
}



void
mufgraph_invalidate (void)
{
  GdkRectangle area;

  if (!GDK_IS_WINDOW (mufgraph_drawing_area->window))
    return;
  
  area = mufgraph_drawing_area->allocation;
  
  area.x = area.y = 0;
  
  gdk_window_invalidate_rect (mufgraph_drawing_area->window, &area, TRUE);
}

void
mufgraph_repaint (cairo_t *cr, int width, int height)
{
  int max_lines;
  
  if (!ionowatch_data_is_ready ())
    return;
    
  if (selected_datasource == scaled_datasource)
  {
    mufgraph_printf (cr, width / 2 - LINE (19), height / 2 - LINE (0.5),
      "To avoid server overhead, MUF graph only works with predicted data");
      
    return;
  }
  
  max_lines = height / MUFGRAPH_FONT_SIZE - 1;
  
  mufgraph_printf (cr, 0, 0, "MUF graph for %sUTC with various skip distances", ctime (&selected_time));
  mufgraph_printf (cr, 0, LINE (1), "Station select: %s (%lgº N %lgº E)", 
    selected_station->name, selected_station->lat, selected_station->lon);
  
  mufgraph_printf (cr, 0, LINE (max_lines - 1), "Datasource select: %s",
    selected_datasource->desc);
    
  mufgraph_draw_grid (cr, LINE (4), LINE (5), 
                          width - LINE (6),
                          height - LINE (10));
                          
  mufgraph_draw_mufs (cr, LINE (4), LINE (5), 
                          width - LINE (6),
                          height - LINE (10));
                          
  
}

void
muf_paint_on_cairo (cairo_t *cr, int width, int height)
{
  cairo_set_source_rgb (cr, 1.0, 0.64706, 0.0);
  
  mufgraph_repaint (cr, width, height);
}

gboolean
mufgraph_expose_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  cairo_t *cr;
  enter_callback_context ();
  
  cr = gdk_cairo_create (GDK_DRAWABLE (widget->window));
  
  cairo_rectangle (cr, event->area.x, event->area.y, event->area.width, event->area.height);
                   
  cairo_clip (cr);
  
  muf_paint_on_cairo (cr, widget->allocation.width, widget->allocation.height);
  
  cairo_destroy (cr);
  
  leave_callback_context ();
  
  return TRUE;
}

