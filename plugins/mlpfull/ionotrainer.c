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
#  define training_set_build predictor_training_set_build
#  define build_mlp predictor_build_mlp
#else

char *default_weight_file = "predictor.mlp";

#endif /* PLUGIN_CODE */

# ifdef USE_LIBFANN

struct fann *
build_fann (void)
{
  return fann_create_standard (5, 6, 6, 8, 16, LAYER_MLP_OUTPUT_COUNT);
}

# else

struct mlp *
build_mlp (void)
{
  return mlp_new (6, 4, 6, 8, 16, LAYER_MLP_OUTPUT_COUNT);
}

# endif /* USE_LIBFANN */

void
training_set_build (struct training_set *set)
{
  int i, j, k;
  numeric_t delta;
  struct layer_info *this;
  numeric_t *new_pattern, *new_output;
  
  set->input_len = 6;
  set->output_len = LAYER_MLP_OUTPUT_COUNT;
  
  for (i = 0; i < set->training_set_count; i++)
    for (j = 0; j < IONOGRAM_MAX_LAYERS; j++)
      if ((this = set->training_set_list[i]->layers[j]) != NULL)
      {
        new_pattern = xmalloc (6 * sizeof (numeric_t));
        new_output = xmalloc (LAYER_MLP_OUTPUT_COUNT * sizeof (numeric_t));
        
        new_output[LAYER_MLP_LOW_FREQ] = this->frequencies[0];
        new_output[LAYER_MLP_HIGH_FREQ] = this->frequencies[this->point_count - 1];
        
        delta = (new_output[LAYER_MLP_HIGH_FREQ] - new_output[LAYER_MLP_LOW_FREQ]) / LAYER_MLP_REL_LAST_HEIGHT;
        
        for (k = 0; k < LAYER_MLP_HEIGHT_COUNT; k++)
        {
          new_output[LAYER_MLP_FIRST_HEIGHT + k] = 
            layer_interpolate_height (this, new_output[LAYER_MLP_LOW_FREQ] + k * delta) / LAYER_HEIGHT_NORMAL_FACTOR;
          
          if (isinf (new_output[LAYER_MLP_FIRST_HEIGHT + k]) || 
              isnan (new_output[LAYER_MLP_FIRST_HEIGHT + k]) ||
              new_output[LAYER_MLP_FIRST_HEIGHT + k] > 1.0 /* This happens when the files are broken */
              )
          {
            ERROR ("damaged vector detected, ignoring\n");
            break;
          }
        }
        
        if (k < LAYER_MLP_HEIGHT_COUNT)
        {
          free (new_pattern);
          free (new_output);
          continue;
        }
        
        new_output[LAYER_MLP_LOW_FREQ] /= PLASMA_FREQUENCY_NORMAL_FACTOR;
        new_output[LAYER_MLP_HIGH_FREQ] /= PLASMA_FREQUENCY_NORMAL_FACTOR;
        
        
        new_pattern [PATTERN_LAT] = set->training_set_list[i]->lat / ANGLE_NORMAL_FACTOR;
        new_pattern [PATTERN_LON] = set->training_set_list[i]->lon / ANGLE_NORMAL_FACTOR;
        new_pattern [PATTERN_INCLINATION] = set->training_set_list[i]->sun_inclination / INCLINATION_NORMAL_FACTOR;
        new_pattern [PATTERN_STATION] = set->training_set_list[i]->solstice_offset / ANGLE_NORMAL_FACTOR;
        new_pattern [PATTERN_SUNSPOTS] = set->training_set_list[i]->sunspot_number / SUNSPOT_NORMAL_FACTOR;
        new_pattern [PATTERN_LAYER] = j / LAYER_NUM_NORMAL_FACTOR;
        
        training_set_add_vectors (set, new_pattern, new_output);
      }
}

