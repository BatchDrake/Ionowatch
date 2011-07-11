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

#ifndef _GUI_PAINTER_H
#define _GUI_PAINTER_H

#include <ionowatch.h>
#include "datasource.h"

#define PAINTER_OPTION_MASK_EVALUATE_ALL 1
#define PAINTER_OPTION_MASK_VERBOSE      2

struct painter
{
  char *name;
  char *desc;
  
  struct ionowatch_plugin *owner;
  struct datasource *datasource;
  struct datasource_magnitude *magnitude;
  struct station_info *station;
  
  time_t time;
  float lat, lon;
  float freq;
  float height;
  float sunspot;
  
  int options;
  
  void (*gl_display_cb) (struct painter *);
  void (*update_model_cb) (struct painter *);
};

struct painter *painter_register (const char *, const char *, 
                  void (*) (struct painter *), void (*) (struct painter *),
                  struct ionowatch_plugin *);
void painter_set_options (struct painter *, int);
void painter_unset_options (struct painter *, int);
void painter_display (struct painter *);
void painter_update_model (struct painter *);
struct station_info * painter_get_station (struct painter *);
void painter_set_station (struct painter *, struct station_info *);
int painter_test_options (struct painter *, int);
void painter_set_datasource (struct painter *, struct datasource *);
void painter_set_magnitude (struct painter *, struct datasource_magnitude *);

void painter_set_latitude (struct painter *, float);
float painter_get_latitude (struct painter *);

void painter_set_longitude (struct painter *, float);
float painter_get_longitude (struct painter *);

void painter_set_sunspot (struct painter *, float);
float painter_get_sunspot (struct painter *);

void painter_set_freq (struct painter *, float);
float painter_get_freq (struct painter *);

void painter_set_height (struct painter *, float);
float painter_get_height (struct painter *);

void painter_set_time (struct painter *, time_t);
time_t painter_get_time (struct painter *);


struct datasource *painter_get_datasource (struct painter *);
struct datasource_magnitude *painter_get_magnitude (struct painter *);

int painter_eval_station (struct painter *, struct station_info *, float *);

void painter_unregister_by_plugin (struct ionowatch_plugin *);


#endif /* _GUI_PAINTER_H */

