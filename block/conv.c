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

#include <mmmfile.h>
#include <rsffile.h>
#include <sbffile.h>
#include <ztdata.h>

#include <index.h>
#include <ionogram.h>

/* TODO: implement here something useful to convert MMM blocks in some
   more manipulable structure. */
   
/* TODO (2): implement this more wisely */

ztdata_t configs;

int
ztdata_load (const char *file, ztdata_t *dest)
{
  FILE *fp;
  char line[120];
  unsigned int z, t;
  int linenum;
  int i;
  
  unsigned int values[8];
  
  if ((fp = fopen (file, "r")) == NULL)
  {
    ERROR ("couldn't open %s: %s\n", file, strerror (errno));
    return -1;
  }
  
  memset (dest, 0, sizeof (ztdata_t));
  
  linenum = 0;
  
  while (!feof (fp))
  {
    linenum++;
    
    if (!fgets (line, 119, fp))
      break;
      
    if (strncmp (line, "zt data", 7) == 0)
    {
      if (sscanf (line, "zt data (%u,%u) = %u,%u,%u,%u,%u,%u,%u,%u;",
        &z, &t, 
        values + 0, values + 1, values + 2, values + 3,
        values + 4, values + 5, values + 6, values + 7) != 10)
      {
        ERROR ("ztdata file %s:%d: malformed line\n", file, linenum);
        continue;
      }
      
      for (i = 0; i < 8; i++)
        (*dest)[z & 7][t & 11][i] = values[i];
    }
  }
  
  return 0;
}


char *
blockfile_build_url (const struct ionogram_filename *fn, const char *baseurl)
{
  return strbuild ("%s/%s/individual/%04d/%03d/ionogram/",
    baseurl, fn->station, fn->year, fn->yday);
    
}


int
ionogram_from_mmm (struct ionogram *dest, FILE *fp)
{
  struct mmm_block block;
  float frequencies[] = {0.2, 0.1, 0.05, 0.025, 0.01, 0.05};
  float heights[] = {0.0, 1.0, 6.0, 16.0};
  float incr[] = {2.5, 5.0, 10.0};
  
  float freq_start, freq_stop, freq_count;
  float height_start, height_stop;
  
  int i, j, n, blocknr;
  
  fseek (fp, 0, SEEK_SET);
  
  if (mmmfile_bread (fp, 0, &block) < 1)
  {
    ERROR ("couldn't read MMM first block\n");
    return -1;
  }
  
  freq_start = nibbles_to_bin (block.preface.start_freq, 2);
  freq_stop  = nibbles_to_bin (block.preface.stop_freq, 2);
  
  freq_count = (freq_stop - freq_start) / 
    frequencies [nibbles_to_bin (block.preface.freq_step, 1)];

  height_start = heights[nibbles_to_bin (block.preface.range_start, 1)] * 10.0;
  height_stop  = height_start + 
    incr[nibbles_to_bin (block.preface.range_increment, 1)] * 
    block.typebyte * 128.0;
    
  ionogram_setup_extended (dest, 
    freq_count, block.typebyte * 128,
    freq_start, freq_stop, height_start, height_stop);
    
  blocknr = n = 0;
  
  /* TODO: include X Scans */
  do
  {
    for (i = 0; 
         i < (3 - block.typebyte) * 15 && n < freq_count; 
         i++, n++)
    {
      for (j = 0; j < block.typebyte * 128; j++)
      {  
        if (block.typebyte == 1)
          dest->o_scans[n][j] = 
            (float) block.groups_1[i].data[j].amplitude / 15.0;
        else if (block.typebyte == 2)
          dest->o_scans[n][j] = 
            (float) block.groups_2[i].data[j].amplitude / 31.0;
        
      }
    }
  }
  while (mmmfile_bread (fp, ++blocknr, &block) == 1);
  
  return 0;
}

int
ionogram_from_rsf (struct ionogram *dest, FILE *fp)
{
  struct rsf_block block;
  int range_bins, heights, groups_per_block;
  float incr;
  
  float freq_start, freq_stop, freq_count;
  float height_start, height_stop, height_count;
  
  int i, j, n, blocknr;
  
  fseek (fp, 0, SEEK_SET);
  
  if (rsffile_bread (fp, 0, &block) < 1)
  {
    ERROR ("couldn't read RSF first block\n");
    return -1;
  }
  
  heights = bcd_to_bin (block.preface.number_of_heights, 4);
  
  if (heights == 128)
  {
    range_bins = 128;
    groups_per_block = 15;
  }
  else if (heights = 256)
  {
    range_bins = 249;
    groups_per_block = 8;
  }
  else if (heights = 512)
  {
    range_bins = 501;
    groups_per_block = 4;
  }
  else
  {
    ERROR ("unsupported number of heights (%d)\n", heights);
    return -1;
  }
    
  freq_start = bcd_to_bin (block.preface.start_freq, 6) / 10000.0;
  freq_stop  = bcd_to_bin (block.preface.stop_freq, 6) / 10000.0;
  
  freq_count = (freq_stop - freq_start) / 
    (bcd_to_bin (block.preface.fine_freq, 4) / 1000.0);

  incr = bcd_to_bin (block.preface.range_incr, 2) == 2 ? 2.5 :
         (float) bcd_to_bin (block.preface.range_incr, 2);
  
  if (incr == 2.0)
    incr = 2.5;
    
  height_start = (float) bcd_to_bin (block.preface.height_window_bottom, 4);
  height_stop  = height_start + incr * range_bins;
  
  range_bins = range_bins * 
    ((float) bcd_to_bin (block.preface.height_window_top, 4) - (float) bcd_to_bin (block.preface.height_window_bottom, 4)) /
    (height_stop - height_start);

  height_stop = (float) bcd_to_bin (block.preface.height_window_top, 4);
  
  ionogram_setup_extended (dest, 
    freq_count, range_bins,
    freq_start, freq_stop, height_start, height_stop);
  
  blocknr = n = 0;
  
  do
  {
    for (i = 0; i < groups_per_block && n < freq_count; i++, n++)
    {
      for (j = 0; j < range_bins; j++)
      {  
      
/* TODO: this if sucks */
        if (heights == 128)
        {
          if (block.groups_128[i].prelude.polarization != 3)
          {
            n--;
            break;
          }
          
          dest->o_scans[n][j] = 
            (float) block.groups_128[i].heights[j].amplitude / 31.0;
        }
        else if (heights == 256)
        {
          if (block.groups_256[i].prelude.polarization != 3)
          {
            n--;
            break;
          }
          
          dest->o_scans[n][j] = 
            (float) block.groups_256[i].heights[j].amplitude / 31.0;
        }
        else if (heights == 512)
        {
          if (block.groups_512[i].prelude.polarization != 3)
          {
            n--;
            break;
          }
          
          dest->o_scans[n][j] = 
            (float) block.groups_512[i].heights[j].amplitude / 31.0;
        }
      }
    }
  }
  while (rsffile_bread (fp, ++blocknr, &block) == 1);
  
  return 0;
}

int
ionogram_from_sbf (struct ionogram *dest, FILE *fp)
{
  struct sbf_block block;
  int range_bins, heights, groups_per_block;
  float incr;
  
  float freq_start, freq_stop, freq_count;
  float height_start, height_stop, height_count;
  
  int i, j, n, blocknr;
  
  fseek (fp, 0, SEEK_SET);
  
  if (sbffile_bread (fp, 0, &block) < 1)
  {
    ERROR ("couldn't read RSF first block\n");
    return -1;
  }
  
  heights = bcd_to_bin (block.preface.number_of_heights, 4);
  
  if (heights == 128)
  {
    range_bins = 128;
    groups_per_block = 30;
  }
  else if (heights = 256)
  {
    range_bins = 256;
    groups_per_block = 15;
  }
  else if (heights = 512)
  {
    range_bins = 498;
    groups_per_block = 8;
  }
  else
  {
    ERROR ("unsupported number of heights (%d)\n", heights);
    return -1;
  }
    
  freq_start = bcd_to_bin (block.preface.start_freq, 6) / 10000.0;
  freq_stop  = bcd_to_bin (block.preface.stop_freq, 6) / 10000.0;
  
  freq_count = (freq_stop - freq_start) / 
    (bcd_to_bin (block.preface.fine_freq, 4) / 1000.0);

  incr = bcd_to_bin (block.preface.range_incr, 2) == 2 ? 2.5 :
         (float) bcd_to_bin (block.preface.range_incr, 2);
  
  if (incr == 2.0)
    incr = 2.5;
    
  height_start = (float) bcd_to_bin (block.preface.height_window_bottom, 4);
  height_stop  = height_start + incr * range_bins;
  
  range_bins = range_bins * 
    ((float) bcd_to_bin (block.preface.height_window_top, 4) - (float) bcd_to_bin (block.preface.height_window_bottom, 4)) /
    (height_stop - height_start);

  height_stop = (float) bcd_to_bin (block.preface.height_window_top, 4);
  
  ionogram_setup_extended (dest, 
    freq_count, range_bins,
    freq_start, freq_stop, height_start, height_stop);
  
  blocknr = n = 0;
  
  do
  {
    for (i = 0; i < groups_per_block && n < freq_count; i++, n++)
    {
      for (j = 0; j < range_bins; j++)
      {  
      
/* TODO: this if sucks */
        if (heights == 128)
        {
          if (block.groups_128[i].prelude.polarization != 3)
          {
            n--;
            break;
          }
          
          dest->o_scans[n][j] = 
            (float) block.groups_128[i].heights[j].amplitude / 31.0;
        }
        else if (heights == 256)
        {
          if (block.groups_256[i].prelude.polarization != 3)
          {
            n--;
            break;
          }
          
          dest->o_scans[n][j] = 
            (float) block.groups_256[i].heights[j].amplitude / 31.0;
        }
        else if (heights == 512)
        {
          if (block.groups_512[i].prelude.polarization != 3)
          {
            n--;
            break;
          }
          
          dest->o_scans[n][j] = 
            (float) block.groups_512[i].heights[j].amplitude / 31.0;
        }
      }
    }
  }
  while (sbffile_bread (fp, ++blocknr, &block) == 1);
  
  return 0;
}


