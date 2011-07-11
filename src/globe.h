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


#ifndef _GLOBE_H
#define _GLOBE_H

#ifndef PI
#  define PI 3.1415926535897932384626433879502884
#endif

#define RAD2DEG(x) ((x) / (2.0 * PI) * 360.0)
#define DEG2RAD(x) ((x) * (2.0 * PI) / 360.0)
#define RADADJUST(x) ((x) - 2.0 * PI * floor ((x) / (2.0 * PI)))

#define SQRT32 0.86603

#define LAT_OFF -PI / 4.0
#define LON_OFF PI / 4.0

/* TODO; include precalculated sines and cosines to improve performance */
#define COS_OF_INCLINATION 0.91749
#define SIN_OF_INCLINATION 0.39776

struct globe_data
{
  int x, y;
  int width, height;
  
  float lat, lon;
  float day_off;
  float sol;
};

float globe_data_get_sun_inclination (struct globe_data *, float, float);
struct globe_data *globe_data_new (int, int, int, int);
void globe_data_destroy (struct globe_data *);
void globe_data_set_time (struct globe_data *, time_t);

#endif /* _GLOBE_H */

