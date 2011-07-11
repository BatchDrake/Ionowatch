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

#define LINE(x) (IONOGRAM_FONT_SIZE * x)

GtkWidget *ionogram_drawing_area;

extern time_t selected_time;
extern struct datasource *selected_datasource;
extern struct datasource_magnitude *selected_magnitude;
extern struct station_info         *selected_station;

extern struct datasource_magnitude *virtual_f2_height;
extern struct datasource_magnitude *virtual_f1_height;
extern struct datasource_magnitude *virtual_e_height;
extern struct datasource_magnitude *virtual_es_height;
extern struct datasource_magnitude *virtual_ea_height;
extern struct datasource_magnitude *plasma_frequency;

extern float  selected_lat;
extern float  selected_lon;
extern float  selected_freq;
extern float  selected_height;
extern float  selected_sunspot;
extern time_t selected_time;

void
ionogram_printf (cairo_t *cr, int x, int y, const char *fmt, ...)
{
  va_list ap;
  char *text;
  cairo_text_extents_t te;
  va_start (ap, fmt);
  
  text = vstrbuild (fmt, ap);
  
  
  cairo_select_font_face (cr, IONOGRAM_FONT_FAMILY,
    CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    
  cairo_set_font_size (cr, IONOGRAM_FONT_SIZE);
  cairo_text_extents (cr, text, &te);
  cairo_move_to (cr, x - te.x_bearing,
    y + IONOGRAM_FONT_SIZE / 2 - te.height / 2 - te.y_bearing);
    
  cairo_show_text (cr, text);
        
  va_end (ap);
}

void
ionogram_draw_grid (cairo_t *cr, int x, int y, int width, int height)
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
  
  for (i = 0; i <= IONOGRAM_GRID_FREQ_STEPS; i++)
  {
    cairo_move_to (cr, x + (float) i / (float) IONOGRAM_GRID_FREQ_STEPS * (width - 1), y);
    cairo_line_to (cr, x + (float) i / (float) IONOGRAM_GRID_FREQ_STEPS * (width - 1), y + height - 1);
    
    if (! (i & 1))
      ionogram_printf (cr, x + (float) i / (float) IONOGRAM_GRID_FREQ_STEPS * (width - 1) - LINE (1), 
        LINE (1) + y + height - 1, "%.1f", i / (float) IONOGRAM_GRID_FREQ_STEPS * IONOGRAM_FREQ_MAX);
  }
  
  for (i = 0; i <= IONOGRAM_GRID_HEIGHT_STEPS; i++)
  {
    cairo_move_to (cr, x, y + (float) i / (float) IONOGRAM_GRID_HEIGHT_STEPS * (height - 1));
    cairo_line_to (cr, x + width - 1, y + (float) i / (float) IONOGRAM_GRID_HEIGHT_STEPS * (height - 1));
    
    ionogram_printf (cr, x - LINE (4), 
                         y + (float) i / (float) IONOGRAM_GRID_HEIGHT_STEPS * (height - 1) - LINE (0.5),
                         "+ %3.0f", IONOGRAM_HEIGHT_MAX - i / (float) IONOGRAM_GRID_HEIGHT_STEPS * IONOGRAM_HEIGHT_MAX);
  }
  
  cairo_stroke (cr);
  
  ionogram_printf (cr, x + width / 2 - LINE (7.5), y + height - 1 + LINE (2),
    "Frequency (MHz)"); 
}

void
ionogram_draw_layers (cairo_t *cr, int x, int y, int width, int height)
{
  struct datasource_magnitude *this_magnitude;
  static const double dashed[] = {1.0};
  
  int i;
  int j;
  
  int last_state, this_state;
  float out;
  
  cairo_set_dash (cr, dashed, 0, 1);
  
  for (i = 0; i < selected_datasource->magnitude_count; i++)
    if ((this_magnitude = selected_datasource->magnitude_list[i]) != NULL)
    {
      if (this_magnitude == virtual_e_height ||
          this_magnitude == virtual_f1_height ||
          this_magnitude == virtual_f2_height ||
          this_magnitude == virtual_es_height ||
          this_magnitude == virtual_ea_height)
      {
        last_state = EVAL_CODE_HOLE;
        
        if (this_magnitude == selected_magnitude)
          cairo_set_source_rgb (cr, 0.0, 1.0, 0.0);
        else
          cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        for (j = 0; j < IONOGRAM_SAMPLES; j++)
        {
          this_state = datasource_eval_station (
            selected_datasource, this_magnitude, selected_station, selected_sunspot, j * IONOGRAM_FREQ_MAX / (float) (IONOGRAM_SAMPLES - 1), 0, selected_time, &out);
          
          if (this_state == EVAL_CODE_DATA)
          {
            if (last_state == EVAL_CODE_DATA)
            {
              cairo_line_to (cr, x + j * (width - 1) / (float) (IONOGRAM_SAMPLES - 1), y + height - 1 - out / IONOGRAM_HEIGHT_MAX * (height - 1));

              cairo_stroke (cr);
            }
              
            cairo_move_to (cr, x + j  * (width - 1) / (float) (IONOGRAM_SAMPLES - 1), y + height - 1 - out / IONOGRAM_HEIGHT_MAX * (height - 1));
          }
          
          last_state = this_state;
        }
        
        if (last_state = EVAL_CODE_DATA)
        {
          cairo_line_to (cr, x + j  * (width - 1) / (float) IONOGRAM_SAMPLES, y + height - 1 - out / IONOGRAM_HEIGHT_MAX * (height - 1));
          cairo_stroke (cr);
        }
      }
    }
}

void
ionogram_draw_plasma_frequency (cairo_t *cr, int x, int y, int width, int height)
{
  struct datasource_magnitude *this_magnitude;
  static const double dashed[] = {1.0};
  
  int i;
  int j;
  
  int last_state, this_state;
  float this_freq;
  float this_height;
  
  cairo_set_dash (cr, dashed, 0, 1);

  last_state = EVAL_CODE_HOLE;
  
  if (plasma_frequency == selected_magnitude)
    cairo_set_source_rgb (cr, 0.0, 1.0, 0.0);
  else
    cairo_set_source_rgb (cr, 0.0, 1.0, 1.0);
    
  for (j = 0; j < IONOGRAM_SAMPLES; j++)
  {
    this_height = IONOGRAM_HEIGHT_MAX * (float) j / (float) (IONOGRAM_SAMPLES - 1);
    
    this_state = datasource_eval_station (
      selected_datasource, plasma_frequency, selected_station, selected_sunspot, 0, this_height, selected_time, &this_freq);
    
    if (this_state == EVAL_CODE_DATA)
    {
      if (last_state == EVAL_CODE_DATA)
      {
        cairo_line_to (cr, x + this_freq / IONOGRAM_FREQ_MAX * (width - 1), y + height - 1 - j * (height - 1) / (float) (IONOGRAM_SAMPLES - 1));

        cairo_stroke (cr);
      }
        
      cairo_move_to (cr, x + this_freq / IONOGRAM_FREQ_MAX * (width - 1), y + height - 1 - j * (height - 1) / (float) (IONOGRAM_SAMPLES - 1));
    }
    
    last_state = this_state;
  }
  
  if (last_state = EVAL_CODE_DATA)
  {
    cairo_line_to (cr, x + this_freq / IONOGRAM_FREQ_MAX * (width - 1), y + height - 1 - j * (height - 1) / (float) (IONOGRAM_SAMPLES - 1));
    cairo_stroke (cr);
  }
}

void
ionogram_invalidate (void)
{
  GdkRectangle area;

  if (!GDK_IS_WINDOW (ionogram_drawing_area->window))
    return;
  
  area = ionogram_drawing_area->allocation;
  
  area.x = area.y = 0;
  
  gdk_window_invalidate_rect (ionogram_drawing_area->window, &area, TRUE);
}

void
ionogram_repaint (cairo_t *cr, int width, int height)
{
  int max_lines;
  
  max_lines = height / IONOGRAM_FONT_SIZE - 1;
  
  ionogram_printf (cr, 0, 0, "Ionogram for %sUTC", ctime (&selected_time));
  ionogram_printf (cr, 0, LINE (1), "Magnitude highlight: %s", 
    selected_magnitude->desc);
  
  ionogram_printf (cr, 0, LINE (max_lines - 1), "Datasource select: %s",
    selected_datasource->desc);
    
  ionogram_draw_grid (cr, LINE (4), LINE (5), 
                          width - LINE (6),
                          height - LINE (10));
                          
  ionogram_draw_layers (cr, LINE (4), LINE (5), 
                          width - LINE (6),
                          height - LINE (10));
                          
  ionogram_draw_plasma_frequency (cr, LINE (4), LINE (5), 
                          width - LINE (6),
                          height - LINE (10));
                          
  
}

void
ionogram_paint_on_cairo (cairo_t *cr, int width, int height)
{
  cairo_set_source_rgb (cr, 1.0, 0.64706, 0.0);
  
  ionogram_repaint (cr, width, height);
}


gboolean
ionogram_expose_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  cairo_t *cr;
  enter_callback_context ();
  
  cr = gdk_cairo_create (GDK_DRAWABLE (widget->window));
  
  cairo_rectangle (cr,
                   event->area.x, event->area.y,
                   event->area.width, event->area.height);
                   
  cairo_clip (cr);
 
  ionogram_paint_on_cairo (cr, widget->allocation.width, widget->allocation.height);
  
  cairo_destroy (cr);
  
  leave_callback_context ();
  
  return TRUE;
}

