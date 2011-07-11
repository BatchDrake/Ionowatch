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

#include "datasource.h"

PTR_LIST (struct datasource_magnitude, global_magnitude);
PTR_LIST (struct datasource, global_datasource);

struct datasource_magnitude *
datasource_magnitude_lookup (const char *name)
{
  int i;
  
  for (i = 0; i < global_magnitude_count; i++)
    if (global_magnitude_list[i] != NULL)
      if (strcmp (global_magnitude_list[i]->name, name) == 0)
        return global_magnitude_list[i];
        
  return NULL;
}

struct datasource_magnitude *
datasource_magnitude_request (const char *name,
                              const char *desc,
                              int type)
{
  struct datasource_magnitude *new;
  
  if ((new = datasource_magnitude_lookup (name)) != NULL)
    return new;
    
  new = xmalloc (sizeof (struct datasource_magnitude));
  
  new->name = xstrdup (name);
  new->desc = xstrdup (desc);
  new->type = type;
  
  PTR_LIST_APPEND (global_magnitude, new);
  
  return new;
}

struct datasource *
datasource_lookup (const char *name)
{
  int i;
  
  for (i = 0; i < global_datasource_count; i++)
    if (global_datasource_list[i] != NULL)
      if (strcmp (global_datasource_list[i]->name, name) == 0)
        return global_datasource_list[i];
    
  return NULL;
}

struct datasource *
datasource_register (const char *name,
                     const char *desc,
                     station_evaluator_cb station_evaluator,
                     void *userdata,
                     struct ionowatch_plugin *plugin)
{
  struct datasource *new;
  
  if (datasource_lookup (name) != NULL)
  {
    /* TODO: implement ionowatch_get_last_error */
    ERROR ("datasource already registered\n"); 
  }
  
  new = xmalloc (sizeof (struct datasource));
  
  memset (new, 0, sizeof (struct datasource));
  
  new->name = xstrdup (name);
  new->desc = xstrdup (desc);
  
  new->station_evaluator = station_evaluator;
  new->userdata = userdata;
  new->owner = plugin;
  
  ionowatch_notify_datasource_list_event ();
  
  PTR_LIST_APPEND (global_datasource, new);
  
  return new;
}

void *
datasource_get_userdata (struct datasource *source)
{
  return source->userdata;
}

void
datasource_unregister_by_plugin (struct ionowatch_plugin *owner)
{
  int i;
  
  for (i = 0; i < global_datasource_count; i++)
    if (global_datasource_list[i] != NULL)
      if (global_datasource_list[i]->owner == owner)
      {
        free (global_datasource_list[i]->name);
        free (global_datasource_list[i]->desc);
        
        if (global_datasource_list[i]->magnitude_list != NULL)
          free (global_datasource_list[i]->magnitude_list);
                
        free (global_datasource_list[i]);
        
        global_datasource_list[i] = NULL;
      }      
  
}

struct datasource_magnitude *
datasource_magnitude_source_lookup (struct datasource *datasource,
                                    const char *name)
{
  int i;
  
  for (i = 0; i < datasource->magnitude_count; i++)
    if (datasource->magnitude_list[i] != NULL)
      if (strcmp (datasource->magnitude_list[i]->name, name) == 0)
        return datasource->magnitude_list[i];
        
  return NULL;
}

void
datasource_register_magnitude (struct datasource *datasource,
                               struct datasource_magnitude *magnitude)
{
  PTR_LIST_APPEND (datasource->magnitude, magnitude);
}


/* It's going to be a pity that every data source must implement its own cache */
int
datasource_eval_station (struct datasource *datasource,
                         struct datasource_magnitude *mag,
                         struct station_info *station,
                         float sunspot,
                         float freq,
                         float height,
                         time_t time, float *out)
{ 
  return (datasource->station_evaluator) 
    (datasource, mag, station, sunspot, freq, height, time, out, datasource->userdata);
}

          
