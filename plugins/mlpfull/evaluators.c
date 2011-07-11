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

#include <pthread.h>
#include <signal.h>
#include <ionowatch.h>
#include <gui.h>
#include <mlp.h>

#include "id3.h"
#include "normal.h"
#include "trainer.h"
#include "evaluators.h"

extern struct id3_parser *parser;
extern struct mlp *mlp;
extern struct mlp *plasma_profile_mlp;
extern struct mlp *h2pf_mlp;

static int
output_makes_sense (float lat, float lon, float incl, float sol, float sunspot, float layer)
{
  float vector[] = {lat, lon, incl, sol, sunspot, layer};
  struct output *result;
  
  result = id3_eval (parser, vector);
  
  return result->number;
  
}

static int
predictor_fill_vector (numeric_t *input_vector, int layer_num, float lat, float lon, float sunspot, time_t date)
{
  struct globe_data globe_data;
  float inclination;
  
  globe_data_set_time  (&globe_data, date);
  
  inclination = globe_data_get_sun_inclination (&globe_data, DEG2RAD (lat), DEG2RAD (-lon));
  
  input_vector[PATTERN_LAT]         = DEG2RAD (lat) / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_LON]         = DEG2RAD (lon) / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_INCLINATION] = inclination / INCLINATION_NORMAL_FACTOR;
  input_vector[PATTERN_STATION]     = globe_data.sol / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_SUNSPOTS]    = sunspot / SUNSPOT_NORMAL_FACTOR;
  input_vector[PATTERN_LAYER]       = (float) layer_num / LAYER_NUM_NORMAL_FACTOR;
  
  return output_makes_sense (
    lat, lon, inclination, RADADJUST (globe_data.sol), sunspot, (float) layer_num);
}

static int
plasma_fill_vector (numeric_t *input_vector, float lat, float lon, float sunspot, time_t date)
{
  struct globe_data globe_data;
  float inclination;
  
  globe_data_set_time  (&globe_data, date);
  
  inclination = globe_data_get_sun_inclination (&globe_data, DEG2RAD (lat), DEG2RAD (-lon));
  
  input_vector[PATTERN_LAT]         = DEG2RAD (lat) / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_LON]         = DEG2RAD (lon) / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_INCLINATION] = inclination / INCLINATION_NORMAL_FACTOR;
  input_vector[PATTERN_STATION]     = globe_data.sol / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_SUNSPOTS]    = sunspot / SUNSPOT_NORMAL_FACTOR;
  
  return 1;
}

static int
h2pf_fill_vector (numeric_t *input_vector, float height, float lat, float lon, float sunspot, time_t date)
{
  struct globe_data globe_data;
  float inclination;
  
  globe_data_set_time  (&globe_data, date);
  
  inclination = globe_data_get_sun_inclination (&globe_data, DEG2RAD (lat), DEG2RAD (-lon));
  
  input_vector[PATTERN_LAT] = DEG2RAD (lat) / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_LON] = DEG2RAD (lon) / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_INCLINATION] = inclination / INCLINATION_NORMAL_FACTOR;
  input_vector[PATTERN_STATION] = globe_data.sol / ANGLE_NORMAL_FACTOR;
  input_vector[PATTERN_SUNSPOTS] = sunspot / SUNSPOT_NORMAL_FACTOR;
  input_vector[PATTERN_HEIGHT] = height / PLASMA_HEIGHT_NORMAL_FACTOR;
  
  return 1;
}

int
real_height_eval (int use_tree, int use_h2pf, int layer_num, float lat, float lon, float sunspot, time_t date, float *out)
{
  numeric_t input_vector[6];
  numeric_t *output_vector;
  
  if (!predictor_fill_vector (input_vector, layer_num, lat, lon, sunspot, date) &&
       use_tree)
    return EVAL_CODE_HOLE;
      
  output_vector = mlp_eval (mlp, input_vector);
  
  *out = LAYER_HEIGHT_NORMAL_FACTOR * output_vector[LAYER_MLP_FIRST_HEIGHT];    
  
  return EVAL_CODE_DATA;
}

int
virtual_height_eval (int use_tree, int use_h2pf, int adjust_freqs, int layer_num, float lat, float lon, float sunspot, float freq, time_t date, float *out)
{
  numeric_t input_vector[6];
  numeric_t *output_vector;
  float plasma_freq;
  float x, factor;
  int i;
  int sample;
  
  if (!predictor_fill_vector (input_vector, layer_num, lat, lon, sunspot, date) &&
       use_tree)
    return EVAL_CODE_HOLE;
      
  
  output_vector = mlp_eval (mlp, input_vector);
  
  for (i = 0; i < LAYER_MLP_OUTPUT_COUNT ; i++)
    if (i < LAYER_MLP_FIRST_HEIGHT)
      output_vector[i] *= PLASMA_FREQUENCY_NORMAL_FACTOR;
    else
      output_vector[i] *= LAYER_HEIGHT_NORMAL_FACTOR;
        
  /* adjust frequency hack: we query the plasma frequency in order to rescalate
     layer frequencies and give a ionogram wich makes sense. */
     
  if (adjust_freqs)
  {
    if (eval_plasma_freq (use_tree, use_h2pf, lat, lon, sunspot, output_vector[LAYER_MLP_FIRST_HEIGHT], date, &plasma_freq) == EVAL_CODE_DATA)
    {
      factor = plasma_freq / output_vector[1];
      
      output_vector[0] *= factor;
      output_vector[1] *= factor;
    }
  }
  
  if (freq < output_vector[LAYER_MLP_LOW_FREQ] || freq > output_vector[LAYER_MLP_HIGH_FREQ])
    return EVAL_CODE_HOLE;
  
  x = (float) LAYER_MLP_REL_LAST_HEIGHT * (freq - output_vector[LAYER_MLP_LOW_FREQ]) / (output_vector[LAYER_MLP_HIGH_FREQ] - output_vector[LAYER_MLP_LOW_FREQ]);
  sample = (int) floor (x);
  factor = x - (float) sample;
    
  if (sample == LAYER_MLP_REL_LAST_HEIGHT)
    *out = output_vector[LAYER_MLP_FIRST_HEIGHT + LAYER_MLP_REL_LAST_HEIGHT];
  else
    *out = (1.0 - factor) * output_vector[LAYER_MLP_FIRST_HEIGHT + sample] + factor * output_vector[LAYER_MLP_FIRST_HEIGHT + sample + 1];
    
  return EVAL_CODE_DATA;

}

static int
h2pf_eval_peak_height (float lat, float lon, float sunspot, time_t date, float *out)
{
  numeric_t input_vector[6];
  numeric_t *output;
  
  int i;
  float max_freq = -INFINITY;
  float height_delta = 10.0;
  float height_max = 0.0;
  if (h2pf_mlp == NULL)
    return EVAL_CODE_NODATA;
    
  for (i = 0; i < (int) (PLASMA_HEIGHT_MAX / height_delta); i++)
  {
    h2pf_fill_vector (input_vector, (float) i * height_delta, lat, lon, sunspot, date);
  
    output = mlp_eval (h2pf_mlp, input_vector);
    
    if (*output > max_freq)
    {
      height_max = (float) i * height_delta;
      max_freq = *output;
    }
  }
  
  
  *out = height_max;
  
  return EVAL_CODE_DATA;
}

int
eval_plasma_peak_height (int use_tree, int use_h2pf, float lat, float lon, float sunspot, time_t date, float *out)
{
  int i;
  float max_freq;
  int   max_id;
  numeric_t input_vector[5];
  numeric_t *output_vector;
  
  if (use_h2pf)
    return h2pf_eval_peak_height (lat, lon, sunspot, date, out);
    
  plasma_fill_vector (input_vector, lat, lon, sunspot, date);
  
  max_id = 0;
  max_freq = -INFINITY;
  
  if (plasma_profile_mlp == NULL)
    return EVAL_CODE_NODATA;
    
  output_vector = mlp_eval (plasma_profile_mlp, input_vector);
  
  for (i = 0; i < PLASMA_MLP_OUTPUT_COUNT; i++)
    if (output_vector[i] > max_freq)
    {
      max_freq = output_vector[i];
      max_id = i;
    }
    
  *out = max_id / (float) PLASMA_MLP_LAST_OUTPUT * PLASMA_HEIGHT_MAX;
  
  return EVAL_CODE_DATA;
}

static int
h2pf_eval_plasma_freq (float lat, float lon, float sunspot, float height, time_t date, float *out)
{
  numeric_t input_vector[6];
  numeric_t *output;
  
  if (h2pf_mlp == NULL)
    return EVAL_CODE_NODATA;
    
  h2pf_fill_vector (input_vector, height, lat, lon, sunspot, date);
  
  output = mlp_eval (h2pf_mlp, input_vector);
  
  *out = *output * PLASMA_FREQUENCY_NORMAL_FACTOR;
  
  return EVAL_CODE_DATA;
}

int
eval_plasma_freq (int use_tree, int use_h2pf, float lat, float lon, float sunspot, float height, time_t date, float *out)
{
  int i, sample;
  
  numeric_t input_vector[5];
  numeric_t *output_vector;
  float x, factor;
 
  if (use_h2pf)
    return h2pf_eval_plasma_freq (lat, lon, sunspot, height, date, out);
    
  plasma_fill_vector (input_vector, lat, lon, sunspot, date);
  
  if (plasma_profile_mlp == NULL)
    return EVAL_CODE_NODATA;
    
  output_vector = mlp_eval (plasma_profile_mlp, input_vector);

  x = (float) PLASMA_MLP_LAST_OUTPUT * height / PLASMA_HEIGHT_MAX;
  
  sample = (int) floor (x);
  factor = x - (float) sample;
    
  if (sample == PLASMA_MLP_LAST_OUTPUT)
    *out = PLASMA_FREQUENCY_NORMAL_FACTOR * output_vector[PLASMA_MLP_LAST_OUTPUT];
  else
    *out = PLASMA_FREQUENCY_NORMAL_FACTOR * ((1.0 - factor) * output_vector[sample] + factor * output_vector[sample + 1]);
  
  return EVAL_CODE_DATA;
}

static int
h2pf_eval_muf (float lat, float lon, float sunspot, time_t date, float *out)
{
  numeric_t input_vector[6];
  numeric_t *output;
  
  int i;
  float max_freq = -INFINITY;
  float height_delta = 10.0;
  float height_max = 0.0;
  if (h2pf_mlp == NULL)
    return EVAL_CODE_NODATA;
    
  for (i = 0; i < (int) (PLASMA_HEIGHT_MAX / height_delta); i++)
  {
    h2pf_fill_vector (input_vector, (float) i * height_delta, lat, lon, sunspot, date);
  
    output = mlp_eval (h2pf_mlp, input_vector);
    
    if (*output > max_freq)
    {
      height_max = (float) i * height_delta;
      max_freq = *output;
    }
  }
  
  
  *out = calc_muf (height_max, max_freq * PLASMA_FREQUENCY_NORMAL_FACTOR);
  
  return EVAL_CODE_DATA;
}


int
eval_muf (int use_tree, int use_h2pf, float lat, float lon, float sunspot, time_t date, float *out)
{
  int i;
  float max_freq;
  int   max_id;
  numeric_t input_vector[5];
  numeric_t *output_vector;
  
  if (use_h2pf)
    return h2pf_eval_muf (lat, lon, sunspot, date, out);
    
  max_id = 0;
  max_freq = -INFINITY;
  
  plasma_fill_vector (input_vector, lat, lon, sunspot, date);
  
  if (plasma_profile_mlp == NULL)
    return EVAL_CODE_NODATA;
    
  output_vector = mlp_eval (plasma_profile_mlp, input_vector);
  
  for (i = 0; i < PLASMA_MLP_OUTPUT_COUNT; i++)
    if (output_vector[i] > max_freq)
    {
      max_freq = output_vector[i];
      max_id = i;
    }
    
  *out = calc_muf (max_id / (float) PLASMA_MLP_LAST_OUTPUT * PLASMA_HEIGHT_MAX, max_freq * PLASMA_FREQUENCY_NORMAL_FACTOR);
  
  return EVAL_CODE_DATA;
}

static int
h2pf_eval_tec (float lat, float lon, float sunspot, time_t date, float *out)
{
  numeric_t input_vector[6];
  numeric_t *output;
  
  int i;
  float height_delta = 10.0;
  float sum;
  
  if (h2pf_mlp == NULL)
    return EVAL_CODE_NODATA;
    
  sum = 0.0;
  
  
  for (i = 0; i < (int) (PLASMA_HEIGHT_MAX / height_delta); i++)
  {
    h2pf_fill_vector (input_vector, (float) i * height_delta, lat, lon, sunspot, date);
  
    output = mlp_eval (h2pf_mlp, input_vector);
    
    sum += height_delta * pow (PLASMA_FREQUENCY_NORMAL_FACTOR * *output / 9.0, 2.0);
  }
  
  *out = sum;
  
  return EVAL_CODE_DATA;
}

int
eval_tec (int use_tree, int use_h2pf, float lat, float lon, float sunspot, time_t date, float *out)
{
  int i;
  float sum, delta;
  float density_1, density_2;
  
  numeric_t input_vector[5];
  numeric_t *output_vector;
  
  if (use_h2pf)
    return h2pf_eval_tec (lat, lon, sunspot, date, out);
    
  plasma_fill_vector (input_vector, lat, lon, sunspot, date);
  
  if (plasma_profile_mlp == NULL)
    return EVAL_CODE_NODATA;
    
  output_vector = mlp_eval (plasma_profile_mlp, input_vector);
  
  sum = 0.0;
  delta = 0.0;
  
  for (i = 1; i < PLASMA_MLP_OUTPUT_COUNT; i++)
  {
    delta = PLASMA_HEIGHT_MAX / (float) PLASMA_MLP_OUTPUT_COUNT;
    
    density_1 = pow (PLASMA_FREQUENCY_NORMAL_FACTOR * output_vector[i - 1] / 9.0, 2.0);
    density_2 = pow (PLASMA_FREQUENCY_NORMAL_FACTOR * output_vector[i] / 9.0, 2.0);
    
    sum += 0.5 * delta * (density_1 + density_2);
  }
   
  *out = sum;
  
  return EVAL_CODE_DATA;
}


