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

#ifndef _BLOCK_H
#define _BLOCK_H

#include <index.h>

#define PACKED __attribute__ ((packed))

#define DECLARE_NIBBLE(x, len) bcdbyte x[len]
#define DECLARE_BCD(x, len) bcdbyte x[len / 2]

#define RECORD_TYPE_SBF_1ST      0x03
#define RECORD_TYPE_SBF          0x02

#define RECORD_TYPE_RSF_1ST      0x07
#define RECORD_TYPE_RSF          0x06

#define RECORD_TYPE_MMM_1ST      0x09
#define RECORD_TYPE_MMM          0x08

typedef unsigned char bcdbyte;

static inline int
nibbles_to_bin (bcdbyte *bcd, int len)
{
  int i;
  int result = 0;
  
  for (i = 0; i < len; i++)
  {
    result *= 10;
    result += bcd[i] & 0x0f;
  }
  
  return result;
}

static inline int
bcd_to_bin (bcdbyte *bcd, int len)
{
  int i;
  int result = 0;
  
  for (i = 0; i < len / 2; i++)
  {
    result *= 10;
    result += bcd[i] >> 4;
    
    result *= 10;
    result += bcd[i] & 0x0f;
  }
  
  return result;
}

struct block_preface
{
  DECLARE_BCD (year, 2);
  DECLARE_BCD (yday, 4);
  DECLARE_BCD (month, 2);
  DECLARE_BCD (mday, 2);
  DECLARE_BCD (hour, 2);
  DECLARE_BCD (min,  2);
  DECLARE_BCD (sec,  2);
  
  char receiver[3];
  char transmitter[3];
  
  DECLARE_BCD (sched, 2);
  DECLARE_BCD (program, 2);
  
  DECLARE_BCD (start_freq, 6);
  DECLARE_BCD (coarse_freq, 4);
  DECLARE_BCD (stop_freq, 6);
  
  DECLARE_BCD (fine_freq, 4);
  
  char small_steps;
  
  DECLARE_BCD (phase_code, 2);
  
  char multi_antenna_seq;
  
  DECLARE_BCD (fft_samples, 2);
  DECLARE_BCD (pulse_repetition_rate, 4);
  DECLARE_BCD (range_start, 4);
  DECLARE_BCD (range_incr, 2);
  DECLARE_BCD (number_of_heights, 4);
  DECLARE_BCD (delay, 4);
  DECLARE_BCD (base_gain, 2);
  DECLARE_BCD (frequency_search, 2);
  DECLARE_BCD (operating_mode, 2);
  DECLARE_BCD (data_format, 2);
  DECLARE_BCD (printer_output, 2);
  DECLARE_BCD (threshold, 2);
  
  char spare[2]; /* Unused */
  
  short cit_len;

  char journal;
  char missing;
  
  DECLARE_BCD (height_window_bottom, 4);
  DECLARE_BCD (height_window_top, 4);
  DECLARE_BCD (heights_to_store, 4);
} PACKED;

static inline void
block_preface_debug (struct block_preface *preface)
{
  NOTICE ("date: %d/%d/%d\n", 
    bcd_to_bin (preface->mday, 2), 
    bcd_to_bin (preface->month, 2), 
    bcd_to_bin (preface->year, 2));
    
  NOTICE ("time: %02d:%02d:%02d\n", 
    bcd_to_bin (preface->hour, 2),
    bcd_to_bin (preface->min, 2),
    bcd_to_bin (preface->sec, 2));
    
  NOTICE ("receiver: %c%c%c\n", 
    preface->receiver[0],
    preface->receiver[1],
    preface->receiver[2]);
    
  NOTICE ("transmitter: %c%c%c\n",
    preface->transmitter[0],
    preface->transmitter[1],
    preface->transmitter[2]);
    
  NOTICE ("freq range: %d00 Hz .. %d00 Hz (coarse step %d kHz)\n", 
    bcd_to_bin (preface->start_freq, 6),
    bcd_to_bin (preface->stop_freq, 6),
    bcd_to_bin (preface->coarse_freq, 4));
    
  NOTICE ("fine frequency step: %d kHz\n", bcd_to_bin (preface->fine_freq, 4));
  
  NOTICE ("number of heights: %d\n", bcd_to_bin (preface->number_of_heights, 4));
  
  NOTICE ("range start: %d km\n", bcd_to_bin (preface->range_start, 4));
  NOTICE ("range increment: %d (encoded)\n", bcd_to_bin (preface->range_incr, 2));
}

char *blockfile_build_url (const struct ionogram_filename *, const char *);
#endif /* _BLOCK_H */

