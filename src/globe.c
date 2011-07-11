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
#include <time.h>
#include <math.h>

#include <util.h>

#include "globe.h"
 
static inline float
project_x (float x, float y, float z)
{
  return x;
} 

static inline float
project_y (float x, float y, float z)
{
  return -z;
} 

static inline float
spherical_to_x (float lat, float lon)
{
  return cos (lat) * cos (lon);
}

static inline float
spherical_to_y (float lat, float lon)
{
  return cos (lat) * sin (lon);
}

static inline float
spherical_to_z (float lat, float lon)
{
  return sin (lat);
}

/*
  Relationship between spherical coordinates and cartesian coordinates can be
  expressed as the basis of the spherical system:
  
  er = sin (lat) * cos (lon) * ex + sin (lat) * cos (lon) * ey + cos (lat) * ez
  el = cos (lat) * cos (lon) * ex + cos (lat) * sin (lon) * ey - sin (lat) * ez
  ep = - sin (lon) * ex + cos (lon) * ey
  
  Where ex, ey and ez are basis for the cartesian coordinate system and
  er, el and ep are basis for the spherical coordinate system.
  
 */


/* TODO: generalize constructors, seriously */
struct globe_data *
globe_data_new (int x, int y, int width, int height)
{
  struct globe_data *data;
  
  data = xmalloc (sizeof (struct globe_data));
  
  data->x = x;
  data->y = y;
  data->width = width;
  data->height = height;
  
  globe_data_set_time (data, time (NULL));
  
  return data;
}

void
globe_data_destroy (struct globe_data *data)
{
  free (data);
}

/* MAGIC: don't touch if you don't want your soul to be sucked down
   to the hell. */
void
globe_data_set_time (struct globe_data *data, time_t date)
{
  data->sol = RADADJUST ((date - 6828960) / 31558149.8 * 2.0 * PI + 3.0 * PI / 2.0);
  data->day_off = RADADJUST (date / 86400.0 * 2.0 * PI);
  data->lon = RADADJUST (PI - data->day_off);  
}

/* There are some details I'm forgetting here, like the fact that the sun is
   not a single point in the sky, but a circle whose diameter is described as
   an angle. This could have an important impact in the calculus of the solar
   incidence.
   */
   
float
globe_data_get_sun_inclination (struct globe_data *data, float lat, float lon)
{
  float y;
  float rx, ry, rz;
  float sin_sol, cos_sol;
  
  sin_sol = sin (data->sol);
  cos_sol = cos (data->sol);
  
  rx = spherical_to_x (lat, lon + data->sol - data->day_off - PI / 2.0);
  ry = spherical_to_y (lat, lon + data->sol - data->day_off - PI / 2.0);
  rz = spherical_to_z (lat, lon + data->sol - data->day_off - PI / 2.0);
  
  y = -rx * sin_sol + ry * COS_OF_INCLINATION * cos_sol + rz * SIN_OF_INCLINATION * cos_sol;
  
  return asin (y);
}

