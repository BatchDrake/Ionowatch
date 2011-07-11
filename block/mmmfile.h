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

#ifndef _MMM_H
#define _MMM_H

#include <util.h>
#include <block.h>

#include <ionogram.h>

struct mmm_preface
{
  DECLARE_NIBBLE (year, 2);
  DECLARE_NIBBLE (yday, 3);
  DECLARE_NIBBLE (hour, 2);
  DECLARE_NIBBLE (min,  2);
  DECLARE_NIBBLE (sec,  2);
  
  DECLARE_NIBBLE (sched, 1);
  DECLARE_NIBBLE (program, 1);
  
  union
  {
    struct
    {
      DECLARE_NIBBLE (journal_1, 1);
      DECLARE_NIBBLE (journal_2, 1);
      DECLARE_NIBBLE (journal_3, 1);
      DECLARE_NIBBLE (journal_4, 1);
      DECLARE_NIBBLE (journal_5, 1);
      DECLARE_NIBBLE (journal_6, 1);
    }
    d256_specific_1;
    
    struct
    {
      DECLARE_NIBBLE (ff_step, 1);
      DECLARE_NIBBLE (journal_2, 1);
      DECLARE_NIBBLE (unused, 1);
      DECLARE_NIBBLE (first_height, 1);
      DECLARE_NIBBLE (height_resolution, 1);
      DECLARE_NIBBLE (number_of_heights, 1);
    }
    dps_specific_1;
  };
  
  DECLARE_NIBBLE (nominal_start_freq, 6);
  
  DECLARE_NIBBLE (out_1, 1);
  DECLARE_NIBBLE (out_2, 1);
  DECLARE_NIBBLE (out_3, 1);
  
  union
  {
    struct
    {
      DECLARE_NIBBLE (printer_cleaning_threshold, 1);
      DECLARE_NIBBLE (printer_amplitude_font, 1);
      DECLARE_NIBBLE (freq_sequencing, 1);
    }
    d256_specific_2;
    
    struct
    {
      DECLARE_NIBBLE (unused2, 1);
      DECLARE_NIBBLE (small_step_nr, 1);
      DECLARE_NIBBLE (multi_antenna_seq, 1);
    }
    dps_specific_2;
  };
  
  DECLARE_NIBBLE (unused3, 1);
  DECLARE_NIBBLE (start_freq, 2);
  DECLARE_NIBBLE (freq_step, 1);
  DECLARE_NIBBLE (stop_freq, 2);
  
  union
  {
    struct
    {
      DECLARE_NIBBLE (trigger, 1);
      DECLARE_NIBBLE (channel_a, 1);
      DECLARE_NIBBLE (channel_b, 1);
    }
    d256_specific_3;
    
    struct
    {
      DECLARE_NIBBLE (hinoise, 1);
      DECLARE_NIBBLE (year4, 1);
      DECLARE_NIBBLE (year3, 1);
    }
    dps_specific_3;
  };
  
  DECLARE_NIBBLE (station, 3);
  DECLARE_NIBBLE (phase_code, 1);
  DECLARE_NIBBLE (azimuth_correction, 1);
  DECLARE_NIBBLE (ant_ss_z, 1);
  DECLARE_NIBBLE (ant_ss_t, 1);
  DECLARE_NIBBLE (sample_num, 1);
  DECLARE_NIBBLE (pulse_rep, 1);
  DECLARE_NIBBLE (pulse_width, 1);
  
  union
  {
    struct
    {
      DECLARE_NIBBLE (time_control, 1);
      DECLARE_NIBBLE (freq_correction, 1);
      DECLARE_NIBBLE (gain_correction, 1);
    }
    d256_specific_4;
    
    struct
    {
      DECLARE_NIBBLE (delay, 1);
      DECLARE_NIBBLE (unused4, 1);
      DECLARE_NIBBLE (ybysed5, 1);
    }
    dps_specific_4;
  };
  
  DECLARE_NIBBLE (range_increment, 1);
  DECLARE_NIBBLE (range_start, 1);
  DECLARE_NIBBLE (frequency_search_enabled, 1);
  DECLARE_NIBBLE (nominal_gain_enabled, 1);
  
} PACKED;

struct type1_data
{
  bcdbyte channel:4;
  bcdbyte amplitude:4;
};

struct type2_data
{
  bcdbyte channel:3;
  bcdbyte amplitude:5;
};


struct mmm_group_1
{
  DECLARE_NIBBLE (type, 1);
  
  bcdbyte freq_6:4;
  bcdbyte freq_7:4;
  
  bcdbyte freq_4:4;
  bcdbyte freq_5:4;
  
  
  bcdbyte gain_setting:4;
  bcdbyte freq_offset:4;
  
  bcdbyte sec_0:4;
  bcdbyte sec_1:4;
  
  bcdbyte mpa;
  
  struct type1_data data[128];
  
} PACKED;

struct mmm_group_2
{
  DECLARE_NIBBLE (type, 1);
  
  bcdbyte freq_6:4;
  bcdbyte freq_7:4;
  
  bcdbyte freq_4:4;
  bcdbyte freq_5:4;
  
  
  bcdbyte gain_setting:4;
  bcdbyte freq_offset:4;
  
  bcdbyte sec_0:4;
  bcdbyte sec_1:4;
  
  
  bcdbyte mpa;
  
  struct type2_data data[256];
  
} PACKED;


struct mmm_block
{
  unsigned char record_type;
  unsigned char header_length;
  unsigned char spare_byte;
  
  struct mmm_preface preface;
  
  union
  {
    char typebyte;
    struct mmm_group_1 groups_1[30];
    struct mmm_group_2 groups_2[15];
  };
  
  char unused[16];
  
} PACKED;

int mmmfile_bread (FILE *, int, struct mmm_block *);
void mmmfile_debug_block (struct mmm_block *);
int ionogram_from_mmm (struct ionogram *, FILE *);

static inline int
mmm_get_amplitude (struct mmm_block *block, int group, int height)
{
  if (block->typebyte == 1)
    return block->groups_1[group].data[height].amplitude;
  else if (block->typebyte == 2)
    return block->groups_2[group].data[height].amplitude;
  else
    return -1;
}

static inline int
mmm_get_channel (struct mmm_block *block, int group, int height)
{
  if (block->typebyte == 1)
    return block->groups_1[group].data[height].channel;
  else if (block->typebyte == 2)
    return block->groups_2[group].data[height].channel;
  else
    return -1;
}

static inline int
channel_to_status (int type, int N, int channel)
{
  static char status_table[4][16] = 
  {
    {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7},
    {8, 9, 10, 11, 12, 13, 14, 15, 4, 5, 6, 7, 0, 1, 2, 3},
    {8, 9, 10, 11, 12, 13, 14, 15, 6, 7, 4, 5, 2, 3, 0, 1},
    {8, 9, 10, 11, 12, 13, 14, 15, 7, 6, 5, 4, 3, 2, 1, 0}
  };
  
  N &= 3;
  channel &= channel;
  
  if (type == 2)
  {
    if (N < 3)
      N++;
      
    channel = ((!!(channel & 4)) << 3) |
              ((!!(channel & 4)) ^ (!!(channel & 2)) << 1) |
              (channel & 1);
  }

  return status_table[N][channel];
}

static inline int
mmm_get_status (struct mmm_block *block, int group, int height)
{
  int c;
  
  if ((c = mmm_get_channel (block, group, height)) == -1)
    return -1;
    
  return channel_to_status (block->typebyte, block->preface.sample_num[0], c);
}


static inline int
get_range_increment (struct mmm_block *block)
{
  return 2500 << bcd_to_bin (block->preface.range_increment, 1);
}

static inline int
get_range_start (struct mmm_block *block)
{
  int ranges[4] = {0, 1, 6, 16};
  
  return ranges[bcd_to_bin (block->preface.range_start, 1) & 3] * 10000;
}

#endif /* _MMM_H */

