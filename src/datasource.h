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

#ifndef _GUI_DATASOURCE_H
#define _GUI_DATASOURCE_H

#include <ionowatch.h>

#define MAGNITUDE_TYPE_HEIGHT            0
#define MAGNITUDE_TYPE_FREQUENCY         1
#define MAGNITUDE_TYPE_REFLECTION        2
#define MAGNITUDE_TYPE_TEC               3

#define EVAL_CODE_DATA   0 /* Data has been retrieved */
#define EVAL_CODE_HOLE   1 /* Data makes nosense here */
#define EVAL_CODE_NODATA 2 /* Data can't be retrieved and it's not likely to be retrieved in the future */
#define EVAL_CODE_WAIT   3 /* The data is not available, but is being retrieved */
#define EVAL_CODE_UNDEF  4 /* Undefined evaluation method */

struct datasource_magnitude
{
  char *name;
  char *desc;
  int type;
};

struct datasource;

typedef int (*station_evaluator_cb) (struct datasource *,
                                     struct datasource_magnitude *,
                                     struct station_info *,
                                     float, float, float, time_t,
                                     float *, void *);
struct datasource
{
  void *userdata;
  char *name;
  char *desc;
  int   new_data_available;
  struct ionowatch_plugin *owner;
  
  PTR_LIST (struct datasource_magnitude, magnitude);
  
  station_evaluator_cb station_evaluator;
                            
};

struct datasource_magnitude *datasource_magnitude_lookup (const char *);
struct datasource_magnitude *datasource_magnitude_request (const char *,
                                                           const char *,
                                                           int);

struct datasource *datasource_lookup (const char *);
struct datasource *datasource_register (const char *, const char *,
                     station_evaluator_cb,
                     void *, struct ionowatch_plugin *);
                     
struct datasource_magnitude *datasource_magnitude_source_lookup (struct datasource *, const char *);
void *datasource_get_userdata (struct datasource *);
void datasource_unregister_by_plugin (struct ionowatch_plugin *);
void datasource_register_magnitude (struct datasource *, struct datasource_magnitude *);
int datasource_eval_station (struct datasource *, struct datasource_magnitude *,
                         struct station_info *, float, float, float, time_t, float *);

                         

#endif /* _GUI_DATASOURCE_H */

