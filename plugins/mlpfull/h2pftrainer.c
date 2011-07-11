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
#  define training_set_build h2pf_training_set_build
#  define build_mlp h2pf_build_mlp
#else

char *default_weight_file = "h2pf.mlp";

#endif /* PLUGIN_CODE */

# ifdef USE_LIBFANN

struct fann *
build_fann (void)
{
  return fann_create_standard (5, 6, 6, 8, 16, H2PF_MLP_OUTPUT_COUNT);
}

# else

struct mlp *
build_mlp (void)
{
  return mlp_new (6, 4, 6, 8, 16, H2PF_MLP_OUTPUT_COUNT);
}

# endif /* USE_LIBFANN */

void
training_set_build (struct training_set *set)
{
  int i, k;
  numeric_t *new_pattern, *new_output;
  
  set->input_len = 6;
  set->output_len = H2PF_MLP_OUTPUT_COUNT;
  
  for (i = 0; i < set->training_set_count; i++)
      for (k = 0; k < set->training_set_list[i]->plasma_scan_count; k++)
      {
        new_pattern = xmalloc (6 * sizeof (numeric_t));
        new_output = xmalloc (H2PF_MLP_OUTPUT_COUNT * sizeof (numeric_t));
        
        new_output[0] = set->training_set_list[i]->plasma_scan_freqs[k] / PLASMA_FREQUENCY_NORMAL_FACTOR;
        
        if (isinf (new_output[0]) || isnan (new_output[0]) || new_output[0] > 1.0)
        {
          ERROR ("invalid pattern (has NaN or non-normalized values)\n");
          ERROR ("THIS IS NOT NORMAL\n");
          abort ();
        }
 
        new_pattern [PATTERN_LAT] = set->training_set_list[i]->lat / ANGLE_NORMAL_FACTOR;
        new_pattern [PATTERN_LON] = set->training_set_list[i]->lon / ANGLE_NORMAL_FACTOR;
        new_pattern [PATTERN_INCLINATION] = set->training_set_list[i]->sun_inclination / INCLINATION_NORMAL_FACTOR;
        new_pattern [PATTERN_STATION] = set->training_set_list[i]->solstice_offset / ANGLE_NORMAL_FACTOR;
        new_pattern [PATTERN_SUNSPOTS] = set->training_set_list[i]->sunspot_number / SUNSPOT_NORMAL_FACTOR;
        new_pattern [PATTERN_HEIGHT] = set->training_set_list[i]->plasma_scan_heights[k] / PLASMA_HEIGHT_NORMAL_FACTOR;
        
        training_set_add_vectors (set, new_pattern, new_output);
      }
}

