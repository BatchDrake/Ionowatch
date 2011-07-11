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

#ifndef _IONOGRAM_H
#define _IONOGRAM_H

#include <math.h>
#include <util.h>

#define IONOGRAM_LAYER_F2      0
#define IONOGRAM_LAYER_F1      1
#define IONOGRAM_LAYER_E       2
#define IONOGRAM_LAYER_ES      3
#define IONOGRAM_LAYER_EA      4

#define IONOGRAM_LAYER_COUNT   5

#define IONOGRAM_POLARIZATION_O 0
#define IONOGRAM_POLARIZATION_X 1

#define IONOGRAM_MAX_LAYERS    (IONOGRAM_LAYER_COUNT << 1)

#define IONOGRAM_LAYER(num, pol) ((num << 1) + pol)

#define IONOGRAM_CANCELLATION_THRESHOLD 0.05

/* TODO: change float for something more configurable */

/* All frequency parameters are in MHz and all distance parameters in Km */

struct layer_info
{
  int polarization;
  int type;
  
  float *heights;
  float *frequencies;
  
  int point_count;
};

struct ionogram
{
  int height_count;
  int freq_count;
  int state_code;
  struct station_info *station;
  float lat, lon;
  float sun_inclination;
  float solstice_offset;
  float sunspot_number;
  
  time_t time;
  
  int   plasma_scan_count;
  float *plasma_scan_heights;
  float *plasma_scan_freqs;
  
  float height_start, height_stop;
  float freq_start, freq_stop;
  
  int     extended;
  
  float **o_scans;
  float **x_scans;
  
  struct layer_info *layers[IONOGRAM_MAX_LAYERS];
};

struct layer_info *layer_info_new (int, int, int);
void layer_info_destroy (struct layer_info *);
struct ionogram *ionogram_new ();
void ionogram_setup_extended 
  (struct ionogram *, int, int, float, float, float, float);
void ionogram_add_layer (struct ionogram *, struct layer_info *);
void ionogram_destroy (struct ionogram *);
void ionogram_normalize_dr (struct ionogram *);

static inline float
ionogram_interpolate_o (struct ionogram *ionogram,
                                float freq, float height)
{
  float i, j, fi, fj, ci, cj;
  
  if (!ionogram->extended)
    return NAN;
    
  if (freq < ionogram->freq_start || freq > ionogram->freq_stop)
    return NAN;
    
  if (height < ionogram->height_start || height > ionogram->height_stop)
    return NAN;
    
  i = (freq - ionogram->freq_start) / (ionogram->freq_stop - ionogram->freq_start);
  j = (height - ionogram->height_start) / (ionogram->height_stop - ionogram->height_start);
  
  fi = floorf (i);
  fj = floorf (j);
  /* What if fi == ci? */
  ci = ceilf (i);
  cj = ceilf (j);
  
  return (cj - j) * (ci - i) * ionogram->o_scans[(int) fi][(int) fj] +
          (cj - j) * (i - fi) * ionogram->o_scans[(int) ci][(int) fj] +
          (j - fj) * (ci - i) * ionogram->o_scans[(int) fi][(int) cj] +
          (j - fj) * (i - fi) * ionogram->o_scans[(int) ci][(int) cj];
}

static inline float
ionogram_interpolate_x (struct ionogram *ionogram,
                                float freq, float height)
{
  float i, j, fi, fj, ci, cj;
  
  if (!ionogram->extended)
    return NAN;
    
  if (freq < ionogram->freq_start || freq > ionogram->freq_stop)
    return NAN;
    
  if (height < ionogram->height_start || height > ionogram->height_stop)
    return NAN;
    
  i = (freq - ionogram->freq_start) / (ionogram->freq_stop - ionogram->freq_start);
  j = (height - ionogram->height_start) / (ionogram->height_stop - ionogram->height_start);
  
  fi = floorf (i);
  fj = floorf (j);
  /* What if fi == ci? */
  ci = ceilf (i);
  cj = ceilf (j);
  
  return (cj - j) * (ci - i) * ionogram->x_scans[(int) fi][(int) fj] +
          (cj - j) * (i - fi) * ionogram->x_scans[(int) ci][(int) fj] +
          (j - fj) * (ci - i) * ionogram->x_scans[(int) fi][(int) cj] +
          (j - fj) * (i - fi) * ionogram->x_scans[(int) ci][(int) cj];
}

static inline float
layer_interpolate_height (struct layer_info *layer, float freq)
{
  int i;
  float range, freq_down, freq_up;
  
  if (freq < layer->frequencies[0])
    return layer->heights[0];
  else if (freq >= layer->frequencies[layer->point_count - 1])
    return layer->heights[layer->point_count - 1];
    
  for (i = 0; i < layer->point_count - 1; i++)
  {
    if (freq >= layer->frequencies[i] && freq < layer->frequencies[i + 1])
    {
      freq_down = layer->frequencies[i];
      freq_up = layer->frequencies[i + 1];
      range =  freq_up - freq_down;
      
      return (freq - freq_down) / range * layer->heights[i + 1] + 
              (freq_up - freq) / range * layer->heights[i];
    }
  } 
  return NAN;
}


static inline float
ionogram_interpolate_plasma_freq (struct ionogram *ionogram,
                                  float height)
{
  int i;
  float range, height_down, height_up;
  
  float height_max, height_min;
  int last_scan;
  
  last_scan = ionogram->plasma_scan_count - 1;
  
  if (last_scan < 0)
    return NAN;
    
  height_min = ionogram->plasma_scan_heights[0];
  height_max = ionogram->plasma_scan_heights[last_scan];
  
  if (height < height_min)
    return 0;
  else if (height > height_max)
    return NAN;
    
  for (i = 0; i < last_scan; i++)
  {
    height_down = ionogram->plasma_scan_heights[i];
    height_up = ionogram->plasma_scan_heights[i + 1];
    if (height >= height_down && 
        height < height_up)
    {
      range =  height_up - height_down;
      
      return (height - height_down) / range * ionogram->plasma_scan_freqs[i + 1] + 
              (height_up - height) / range * ionogram->plasma_scan_freqs[i];
    }
  } 
  return NAN;
}

static inline float
ionogram_index_to_freq (struct ionogram *ionogram, int index)
{   
  if (index < 0 || index >= ionogram->freq_count)
    return NAN;
    
  return ionogram->freq_start + (ionogram->freq_stop - ionogram->freq_start) * (float) index / (float) (ionogram->freq_count - 1);
}

static inline float
ionogram_plasma_index_to_height (struct ionogram *ionogram, int index)
{   
  if (index < 0 || index >= ionogram->height_count)
    return NAN;
    
  return ionogram->height_start + (ionogram->height_stop - ionogram->height_start) * (float) index / (float) (ionogram->height_count - 1);
}



#endif /* _IONOGRAM_H */

