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
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include "trainer.h"

#ifdef PLUGIN_CODE
#  define training_set_build plasma_training_set_build
#  define build_mlp plasma_build_mlp
#else

char *default_weight_file = "plasma.mlp";

#endif /* PLUGIN_CODE */

# ifdef USE_LIBFANN

struct fann *
build_fann (void)
{
  return fann_create_standard (5, 5, 6, 8, 16, PLASMA_MLP_OUTPUT_COUNT);
}

# else

struct mlp *
build_mlp (void)
{
  return mlp_new (5, 4, 6, 8, 16, PLASMA_MLP_OUTPUT_COUNT);
}

# endif /* USE_LIBFANN */

void
training_set_build (struct training_set *set)
{
  int i, k;
  numeric_t *new_pattern, *new_output;
  
  set->input_len = 5;
  set->output_len = PLASMA_MLP_OUTPUT_COUNT;
  
  for (i = 0; i < set->training_set_count; i++)
    if (set->training_set_list[i]->plasma_scan_count > 0)
    {
      new_pattern = xmalloc (5 * sizeof (numeric_t));
      new_output = xmalloc (PLASMA_MLP_OUTPUT_COUNT * sizeof (numeric_t));
      
      for (k = 0; k < PLASMA_MLP_OUTPUT_COUNT; k++)
      {
        new_output[k] = 
          ionogram_interpolate_plasma_freq
            (set->training_set_list[i], 
             (float) k / PLASMA_MLP_LAST_OUTPUT * PLASMA_HEIGHT_MAX) / PLASMA_FREQUENCY_NORMAL_FACTOR;
        
        if (isinf (new_output[k]) || isnan (new_output[k]) || new_output[k] > 1.0)
        {
          ERROR ("invalid pattern (has NaN or non-normalized values)\n");
          break;
        }
      }
      
      if (k < PLASMA_MLP_OUTPUT_COUNT)
      {
        free (new_pattern);
        free (new_output);
        continue;
      }
      
      new_pattern [PATTERN_LAT] = set->training_set_list[i]->lat / ANGLE_NORMAL_FACTOR;
      new_pattern [PATTERN_LON] = set->training_set_list[i]->lon / ANGLE_NORMAL_FACTOR;
      new_pattern [PATTERN_INCLINATION] = set->training_set_list[i]->sun_inclination / INCLINATION_NORMAL_FACTOR;
      new_pattern [PATTERN_STATION] = set->training_set_list[i]->solstice_offset / ANGLE_NORMAL_FACTOR;
      new_pattern [PATTERN_SUNSPOTS] = set->training_set_list[i]->sunspot_number / SUNSPOT_NORMAL_FACTOR;
      
      training_set_add_vectors (set, new_pattern, new_output);
    }
}

