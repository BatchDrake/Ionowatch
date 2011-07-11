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

#include <util.h>
#include <mmmfile.h>

#include <ionogram.h>

void
mmmfile_debug_block (struct mmm_block *block)
{
  int yday, year;
  
  int i;
  
  
  if (block->record_type != RECORD_TYPE_MMM_1ST &&
      block->record_type != RECORD_TYPE_MMM)
  {
    NOTICE ("unknown record type <%02x>\n", block->record_type);
    return;
  }
  
  
  NOTICE ("RECORD_TYPE: %02x\n", block->record_type);
  NOTICE ("HEADER_LENGTH: %02x\n", block->header_length);
  NOTICE ("SPARE_BYTE: %02x\n", block->spare_byte);
  NOTICE ("-----8<-------------------------------------\n");
  NOTICE ("Block preface information:\n");
  
  yday = nibbles_to_bin (block->preface.yday, 3);
  year = nibbles_to_bin (block->preface.year, 2);
  
  NOTICE ("Date: %d/%d/%d %2d:%02d:%02d\n",
    1 + (yday_to_daymonth (yday, year + 2000) & 31),
    1 + (yday_to_daymonth (yday, year + 2000) >> 5),
    year + 2000,
    nibbles_to_bin (block->preface.hour, 2),
    nibbles_to_bin (block->preface.min, 2),
    nibbles_to_bin (block->preface.sec, 2)
    );
  NOTICE ("Schedule #%d, program #%d\n", 
    nibbles_to_bin (block->preface.sched, 1),
    nibbles_to_bin (block->preface.program, 1));
    
  NOTICE ("Nominal start freq: %d.%d KHz\n", 
    nibbles_to_bin (block->preface.nominal_start_freq, 6) / 10,
    nibbles_to_bin (block->preface.nominal_start_freq, 6) % 10);
  NOTICE ("Output control nibbles: %X/%X/%X\n",
    nibbles_to_bin (block->preface.out_1, 1),
    nibbles_to_bin (block->preface.out_2, 1),
    nibbles_to_bin (block->preface.out_3, 1));
  
  NOTICE ("Frequency parameters (MHz): %d..%d..%d\n", 
    nibbles_to_bin (block->preface.start_freq, 2),
    nibbles_to_bin (block->preface.freq_step, 1),
    nibbles_to_bin (block->preface.stop_freq, 2));
    
  NOTICE ("Station %d, phase code %d, azimuth correction %d\n",
    nibbles_to_bin (block->preface.station, 3),
    nibbles_to_bin (block->preface.phase_code, 1),
    nibbles_to_bin (block->preface.azimuth_correction, 1));
    
  NOTICE ("Antenna sequencing: Z = %X, T = %X, %d samples\n",
    nibbles_to_bin (block->preface.ant_ss_z, 1),
    nibbles_to_bin (block->preface.ant_ss_t, 1),
    nibbles_to_bin (block->preface.sample_num, 1));
    
  NOTICE ("Pulse repetition rate: %d, width %d\n", 
    nibbles_to_bin (block->preface.pulse_rep, 1),
    nibbles_to_bin (block->preface.pulse_width, 1));
  
  NOTICE ("Range increment: (%d) %d, start: %d\n",
    nibbles_to_bin (block->preface.range_increment, 1) >> 3,
    nibbles_to_bin (block->preface.range_increment, 1) & 7,
    nibbles_to_bin (block->preface.range_start, 1));
  
  NOTICE ("Frequency Search: %s, Nominal Gain: %s\n", 
    nibbles_to_bin (block->preface.frequency_search_enabled, 1) ? "yes" : "no",
    nibbles_to_bin (block->preface.nominal_gain_enabled, 1) ? "yes" : "no");
    
  NOTICE ("This block holds type-%d groups\n", block->typebyte);
  NOTICE ("\n");
  NOTICE ("Group # | Frequency  |  I |  G | Time\n");
  NOTICE ("--------+------------+----+----+---------------\n");
  
  if (block->typebyte == 1)
  {
    for (i = 0; i < 30; i++)
    {
      NOTICE ("     %2d | %d%d.%d%d0 KHz | %2d | %2d | %d%d s\n",
        i,
        block->groups_1[i].freq_7,
        block->groups_1[i].freq_6,
        block->groups_1[i].freq_5,
        block->groups_1[i].freq_4,
        block->groups_1[i].freq_offset,
        block->groups_1[i].gain_setting,
        block->groups_1[i].sec_1,
        block->groups_1[i].sec_0);
        
    }
  }
  else  if (block->typebyte == 2)
  {
    for (i = 0; i < 15; i++)
    {
      NOTICE ("     %2d | %d%d.%d%d0 KHz | %2d | %2d | %d%d s\n",
        i,
        block->groups_2[i].freq_7,
        block->groups_2[i].freq_6,
        block->groups_2[i].freq_5,
        block->groups_2[i].freq_4,
        block->groups_2[i].freq_offset,
        block->groups_2[i].gain_setting,
        block->groups_2[i].sec_1,
        block->groups_2[i].sec_0);
        
    }
  }
}

int
mmmfile_bread (FILE *fp, int blocknr, struct mmm_block *block)
{
  if (block < 0)
  {
    ERROR ("negative block requested (very likely to be a bug)\n");
    return 0;
  }
  
  fseek (fp, blocknr * sizeof (struct mmm_block), SEEK_SET);
  
  errno = 0;
  
  if (fread (block, sizeof (struct mmm_block), 1, fp) < 1)
    return 0;
  
  if ((block->record_type != RECORD_TYPE_MMM_1ST && block == 0) &&
      (block->record_type != RECORD_TYPE_MMM     && block != 0))
  {
    ERROR ("invalid block read, invalid MMM file (record type %02x unknown)\n",
      block->record_type);
    return -1;
  }
  
  return 1;
}


