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
#include <stdlib.h>
#include <string.h>

#include "painter.h"

PTR_LIST (struct painter, painter);

struct painter *
painter_register (const char *name, const char *desc, 
                  void (*update_model_cb) (struct painter *),
                  void (*gl_display_cb) (struct painter *),
                  struct ionowatch_plugin *owner)
{
  struct painter *new;
  
  new = xmalloc (sizeof (struct painter));
  
  memset (new, 0, sizeof (struct painter));
  
  new->name = xstrdup (name);
  new->desc = xstrdup (desc);
  new->gl_display_cb = gl_display_cb;
  new->update_model_cb = update_model_cb;
  new->owner = owner;
  
  PTR_LIST_APPEND (painter, new);
  
  return new;
}

void
painter_set_options (struct painter *painter, int options)
{
  painter->options |= options;
}

void
painter_unset_options (struct painter *painter, int options)
{
  painter->options &= ~options;
}

void
painter_display (struct painter *painter)
{
  if (painter->gl_display_cb != NULL)
    (painter->gl_display_cb) (painter);
}


void
painter_update_model (struct painter *painter)
{
  if (painter->update_model_cb != NULL)
    (painter->update_model_cb) (painter);
  
  gl_globe_notify_repaint ();
}

struct station_info *
painter_get_station (struct painter *painter)
{
  return painter->station;
}

void
painter_set_station (struct painter *painter, struct station_info *station)
{
  painter->station = station;
}

int
painter_test_options (struct painter *painter, int opts)
{
  return !! (opts & painter->options);
}

void
painter_set_datasource (struct painter *painter, struct datasource *source)
{
  painter->datasource = source;
  painter->magnitude = NULL;
}

void
painter_set_magnitude (struct painter *painter, struct datasource_magnitude *magnitude)
{
  painter->magnitude = magnitude;
}

struct datasource *
painter_get_datasource (struct painter *painter)
{
  return painter->datasource;
}

struct datasource_magnitude *
painter_get_magnitude (struct painter *painter)
{
  return painter->magnitude;
}

void
painter_set_latitude (struct painter *painter, float val)
{
  painter->lat = val;
}

float
painter_get_latitude (struct painter *painter)
{
  return painter->lat;
}

void
painter_set_longitude (struct painter *painter, float val)
{
  painter->lon = val;
}

float
painter_get_longitude (struct painter *painter)
{
  return painter->lon;
}

void
painter_set_sunspot (struct painter *painter, float val)
{
  painter->sunspot = val;
}

float
painter_get_sunspot (struct painter *painter)
{
  return painter->sunspot;
}

void
painter_set_freq (struct painter *painter, float val)
{
  painter->freq = val;
}

float
painter_get_freq (struct painter *painter)
{
  return painter->freq;
}

void
painter_set_height (struct painter *painter, float val)
{
  painter->height = val;
}

float
painter_get_height (struct painter *painter)
{
  return painter->height;
}

void
painter_set_time (struct painter *painter, time_t val)
{
  painter->time = val;
}

time_t
painter_get_time (struct painter *painter)
{
  return painter->time;
}

int
painter_eval_station (struct painter *painter, struct station_info *info, float *out)
{
  return datasource_eval_station 
    (painter->datasource, 
     painter->magnitude, 
     info, 
     painter->sunspot, 
     painter->freq, 
     painter->height,
     painter->time, 
     out);
}

void
painter_unregister_by_plugin (struct ionowatch_plugin *owner)
{
  int i;
  
  for (i = 0; i < painter_count; i++)
    if (painter_list[i] != NULL)
      if (painter_list[i]->owner == owner)
      {
        free (painter_list[i]->name);
        free (painter_list[i]->desc);
        free (painter_list[i]);
        painter_list[i] = NULL;
      }
}

