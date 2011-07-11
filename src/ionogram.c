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
#include <ionowatch.h>

struct layer_info *
layer_info_new (int type, int polarization, int points)
{
  struct layer_info *new;
  int i;
  
  new = xmalloc (sizeof (struct layer_info));
  
  memset (new, 0, sizeof (struct layer_info));
  
  new->heights = xmalloc (points * sizeof (float));
  new->frequencies = xmalloc (points * sizeof (float));
  
  new->point_count = points;
 
  new->type = type;
  new->polarization = polarization;
  
  for (i = 0; i < points; i++)
    new->heights[i] = new->frequencies[i] = 0.0;
    
  return new;
}

void
layer_info_destroy (struct layer_info *info)
{
  free (info->heights);
  free (info->frequencies);
  
  free (info);
}

struct ionogram *
ionogram_new (void)
{
  struct ionogram *new;
  int i, j;
  
  new = xmalloc (sizeof (struct ionogram));
  
  memset (new, 0, sizeof (struct ionogram));
  
  return new;
}

void
ionogram_setup_extended (struct ionogram *ionogram,
                         int freq_samples, int height_samples,
                         float freq_start,
                         float freq_stop,
                         float height_start,
                         float height_stop)
{
  int i, j;
  
  if (ionogram->extended)
  {
    WARNING ("ionogram is already extended, not doing anything\n");
    return;
  }
  
  ionogram->freq_count = freq_samples;
  ionogram->height_count = height_samples;
  
  ionogram->o_scans = xmalloc (sizeof (float *) * freq_samples);
  ionogram->x_scans = xmalloc (sizeof (float *) * freq_samples);
  
  for (i = 0; i < freq_samples; i++)
  {
    ionogram->o_scans[i] = xmalloc (sizeof (float) * height_samples);
    ionogram->x_scans[i] = xmalloc (sizeof (float) * height_samples);
    
    for (j = 0; j < height_samples; j++)
      ionogram->o_scans[i][j] = ionogram->x_scans[i][j] = NAN;
  }
  
  ionogram->freq_start = freq_start;
  ionogram->freq_stop  = freq_stop;
  
  ionogram->height_start = height_start;
  ionogram->height_stop  = height_stop;
  
  ionogram->extended = 1;
  
  return;
}

void
ionogram_add_layer (struct ionogram *ionogram, struct layer_info *info)
{
  if (ionogram->layers[IONOGRAM_LAYER (info->type, info->polarization)] != NULL)
  {
    ERROR ("layer %d already recorded!\n", info->type);
    return;
  }
  
  ionogram->layers[IONOGRAM_LAYER (info->type, info->polarization)] = info;
}

void
ionogram_destroy (struct ionogram *ionogram)
{
  int i;
  
  if (ionogram->extended)
  {
    for (i = 0; i < ionogram->freq_count; i++)
    {
      free (ionogram->o_scans[i]);
      free (ionogram->x_scans[i]);
    }
    
    free (ionogram->o_scans);
    free (ionogram->x_scans);
  }
  
  if (ionogram->plasma_scan_count > 0)
    free (ionogram->plasma_scan_freqs);
  
  for (i = 0; i < IONOGRAM_MAX_LAYERS; i++)
    if (ionogram->layers[i] != NULL)
      layer_info_destroy (ionogram->layers[i]);

    
  free (ionogram);
}

void
ionogram_normalize_dr (struct ionogram *ionogram)
{
  int i, j;
  float omax, oavg, xmax, xavg;
  
  if (!ionogram->extended)
    return;
    
  for (i = 0; i < ionogram->freq_count; i++)
  {
    xavg = oavg = 0.0;
    
    for (j = 0; j < ionogram->height_count; j++)
    {
      oavg += ionogram->o_scans[i][j] / (float) ionogram->height_count;
      xavg += ionogram->x_scans[i][j] / (float) ionogram->height_count;
    }
    
    for (j = 0; j < ionogram->height_count; j++)
    {
      ionogram->o_scans[i][j] -= (oavg + IONOGRAM_CANCELLATION_THRESHOLD);
      if (ionogram->o_scans[i][j] < 0.0)
        ionogram->o_scans[i][j] = 0.0;
        
      ionogram->x_scans[i][j] -= (xavg + IONOGRAM_CANCELLATION_THRESHOLD);
      if (ionogram->x_scans[i][j] < 0.0)
        ionogram->x_scans[i][j] = 0.0; 
    }
  }
  
  xmax = omax = 0.0;
    
  for (i = 0; i < ionogram->freq_count; i++)
    for (j = 0; j < ionogram->height_count; j++)
    {
      if (ionogram->o_scans[i][j] > omax)
        omax = ionogram->o_scans[i][j];
      
      if (ionogram->x_scans[i][j] > xmax)
        xmax = ionogram->x_scans[i][j];
    }
  
  for (i = 0; i < ionogram->freq_count; i++)
    for (j = 0; j < ionogram->height_count; j++)
    {
      ionogram->o_scans[i][j] /= omax;
      ionogram->x_scans[i][j] /= xmax;
    }
}


