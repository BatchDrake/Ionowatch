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

#include "gui.h"

#include <GL/gl.h>
#include <GL/glut.h>

struct model_data
{
  struct station_info *info;
  struct datasource_magnitude *magnitude;
  float radius;
  float depth;
  float value;
};

PTR_LIST (struct model_data, model_data);
PTR_LIST_EXTERN (struct station_info, stations);

void
model_data_add (struct station_info *info, struct datasource_magnitude *magnitude, float value)
{
  struct model_data *new;
  
  new = xmalloc (sizeof (struct model_data));
  
  new->info = info;
  new->value = value;
  new->magnitude = magnitude;
  
  if (magnitude->type == MAGNITUDE_TYPE_REFLECTION)
  {
    new->radius = earth_get_hop_radius (value);
    new->depth  = earth_get_hop_depth (value);
  }
  
  PTR_LIST_APPEND (model_data, new);
}

void
model_data_clear (void)
{
  int i;
  
  if (model_data_count > 0)
  {
    for (i = 0; i < model_data_count; i++)
      free (model_data_list[i]);
      
    free (model_data_list);
    model_data_list = NULL;
    model_data_count = 0;
  }
}

void
default_update_model (struct painter *painter)
{
  int pending = 0;
  int full = 0;
  int result;
  float value;
  int i;
  
  struct datasource *source;
  struct datasource_magnitude *magnitude;
 
  source = painter_get_datasource (painter);
  magnitude = painter_get_magnitude (painter);
     
  if (source == NULL || magnitude == NULL)
    return;
 
  model_data_clear ();
  
  if (painter_test_options (painter, PAINTER_OPTION_MASK_EVALUATE_ALL))
  {
    for (i = 0; i < stations_count; i++)
      if (stations_list[i] != NULL)
      {
        result = painter_eval_station (painter, stations_list[i], &value);
        
        if (result == EVAL_CODE_DATA)
        {
          model_data_add (stations_list[i], magnitude, value);
          
          full++;
        }
        else if (result == EVAL_CODE_WAIT)
          pending++;
      }
  }
  else
  {
    result = painter_eval_station (painter, painter_get_station (painter), &value);
    if (result == EVAL_CODE_DATA)
    {
      model_data_add (painter_get_station (painter), magnitude, value);
      full++;
    }
    else if (result == EVAL_CODE_WAIT)
      pending++;
  }
  
  ionowatch_set_status ("%d stations evaluated, %d pending", full, pending);
}

#define BLUE(x)  (x)
#define GREEN(x) ((x) << 8)
#define RED(x)   ((x) << 16)

#define G_RED(color)   (((color) >> 16) & 0x0ff)  
#define G_GREEN(color) (((color) >> 8) & 0x0ff) 
#define G_BLUE(color)  ((color) & 0x0ff)         

#define CALC_COLOR_CYCLE 1536

static inline int 
calc_color (int color)
{
  int rcolor;

  if (color < 0)
    color = -color;
    
  rcolor = color % CALC_COLOR_CYCLE;
	
  if (rcolor < 256)
  {
    if (color == rcolor)
      return RED (rcolor);
      
    return (RED (255) | BLUE (255 - rcolor));
  } 
  else if (rcolor < 256 * 2)
    return (RED (255) | GREEN (255 - ((256 * 2 - 1) - rcolor)));
  else if (rcolor < 256 * 3)
    return (RED ((256 * 3 - 1) - rcolor) | GREEN (255));
  else if (rcolor < 256 * 4)
    return (GREEN (255) | BLUE (255 - ((256 * 4 - 1) - rcolor)));
  else if (rcolor < 256 * 5)
    return (GREEN ((256 * 5 - 1) - rcolor) | BLUE (255));
  else if (rcolor < 256 * 6)
    return (BLUE (255) | RED (255 - ((256 * 6 - 1) - rcolor)));
  else
    return -1;
	
}


void
default_display (struct painter *painter)
{
  int i;
  float factor;
  int color;
  
  glDisable (GL_LIGHTING);
  
  for (i = 0; i < model_data_count; i++)
    if (model_data_list[i] != NULL)
    {
      glPushMatrix ();
    
      if (model_data_list[i]->info == painter->station)
        glColor3f (1.0, 0.0, 0.0);
      else
        glColor3f (1.0, 0.64, 0.0);
        
      gl_globe_rotate_to (model_data_list[i]->info->lat, model_data_list[i]->info->lon);
      
      glTranslatef (0.0, 0.0, 1.0);
      
      if (model_data_list[i]->magnitude->type == MAGNITUDE_TYPE_REFLECTION)
      {
        glPushMatrix ();
        
        glTranslatef (0.0, 0.0, model_data_list[i]->depth - 1.0);
        glRotatef (90.0, 1.0, 0.0, 0.0);
        draw_circle (0.0, 0.0, 0.0, model_data_list[i]->radius + 0.01);
        
        glPopMatrix ();
      }
      
      if (model_data_list[i]->magnitude->type == MAGNITUDE_TYPE_FREQUENCY)
      {
        factor = model_data_list[i]->value;
        
        if (factor > 100.0)
          factor = 100.0;
          
        color = calc_color (2 * CALC_COLOR_CYCLE - (factor / 100.0 * CALC_COLOR_CYCLE));
        
        glColor3f (G_RED (color) / 255.0, G_GREEN (color) / 255.0, G_BLUE (color) / 255.0);
        glutSolidSphere (0.025, 10, 10);
      }
      else if (model_data_list[i]->magnitude->type == MAGNITUDE_TYPE_TEC)
      {
        factor = model_data_list[i]->value;
        
        if (factor > 250.0)
          factor = 250.0;
          
        color = calc_color (2 * CALC_COLOR_CYCLE - (factor / 250.0 * CALC_COLOR_CYCLE));
        
        glColor3f (G_RED (color) / 255.0, G_GREEN (color) / 255.0, G_BLUE (color) / 255.0);
        glutSolidSphere (0.025, 10, 10);
      }
      else
      {
        glTranslatef (0.0, 0.0, model_data_list[i]->value / EARTH_RADIUS);
        glutSolidCube (0.025);
      }
      
      if (painter_test_options (painter, PAINTER_OPTION_MASK_VERBOSE))
      {
        // glRotatef (90.0, 0.0, 1.0, 0.0);
        gl_printf (0.025, 0, "%s (%s) %s: %lg\n", model_data_list[i]->info->name, model_data_list[i]->info->name_long, model_data_list[i]->magnitude->desc, model_data_list[i]->value);
      }
      
      glPopMatrix ();
    }    
  
  glEnable (GL_LIGHTING);
  
}

void
init_default_painter (void)
{
  painter_register ("default", "Builtin display style", default_update_model, default_display, NULL);
}

