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

#ifndef _NORMAL_H
#define _NORMAL_H

#include <ionogram.h>

#define PATTERN_LAT         0
#define PATTERN_LON         1
#define PATTERN_INCLINATION 2
#define PATTERN_STATION     3
#define PATTERN_SUNSPOTS    4
#define PATTERN_LAYER       5

#define PATTERN_HEIGHT      5

#define DESIRED_MSE         0.036
#define DEFAULT_MLP_TRAINING_SPEED 0.0001

# ifndef PI
# define PI 3.14159265358979323846264338327950288419716939937510
# endif


#define ANGLE_NORMAL_FACTOR            (2.0 * PI)
#define INCLINATION_NORMAL_FACTOR      2.0
#define SUNSPOT_NORMAL_FACTOR          200.0
#define PLASMA_FREQUENCY_NORMAL_FACTOR 20.0
#define PLASMA_HEIGHT_MAX              900.0
#define LAYER_NUM_NORMAL_FACTOR        (2.0 * (float) IONOGRAM_MAX_LAYERS)
#define LAYER_HEIGHT_NORMAL_FACTOR     700.0
#define PLASMA_HEIGHT_NORMAL_FACTOR    1800.0

#define H2PF_MLP_OUTPUT_COUNT          1

#define PLASMA_MLP_OUTPUT_COUNT        32
#define PLASMA_MLP_LAST_OUTPUT         (PLASMA_MLP_OUTPUT_COUNT - 1)

#define LAYER_MLP_OUTPUT_COUNT         32
#define LAYER_MLP_LAST_OUTPUT          (LAYER_MLP_OUTPUT_COUNT - 1)

#define LAYER_MLP_LOW_FREQ             0
#define LAYER_MLP_HIGH_FREQ            1
#define LAYER_MLP_FIRST_HEIGHT         2
#define LAYER_MLP_HEIGHT_COUNT         (LAYER_MLP_OUTPUT_COUNT - LAYER_MLP_FIRST_HEIGHT)
#define LAYER_MLP_REL_LAST_HEIGHT      (LAYER_MLP_LAST_OUTPUT - LAYER_MLP_FIRST_HEIGHT)

#endif /* _NORMAL_H */


