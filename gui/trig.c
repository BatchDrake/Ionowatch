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

#include <math.h>

#include "trig.h"

float
calc_muf (float peak_height, float plasma_freq)
{
  peak_height /= EARTH_RADIUS;
  
  return plasma_freq / sqrt (1.0 - pow (1.0 / (1.0 + peak_height), 2.0));
}

float
earth_get_hop_radius (float height)
{
  height /= EARTH_RADIUS;
  
  return sqrt (1.0 - pow (1.0 + height, -2.0));
}

float
earth_get_hop_depth (float height)
{
  height /= EARTH_RADIUS;
  
  return 1.0 / (1.0 + height);
}

