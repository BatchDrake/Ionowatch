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

#ifndef _RSFFILE_H
#define _RSFFILE_H

#include <util.h>
#include <block.h>

#include <ionogram.h>

struct rsf_group_prelude
{
  char group_size:4;
  char polarization:4;
  
  
  DECLARE_BCD (frequency_reading, 4);
  
  char additional_gain:4;
  char offset:4;
  
  
  DECLARE_BCD (seconds, 2);
  DECLARE_BCD (mpa, 2);
  
} PACKED;

struct rsf_range_bin
{
  char doppler_number:3;
  unsigned char amplitude:5;
  
  char azimuth:3;
  char phase:5;
  
} PACKED;

struct rsf_group_128
{
  struct rsf_group_prelude prelude;
  
  struct rsf_range_bin heights[128];
} PACKED;


struct rsf_group_256
{
  struct rsf_group_prelude prelude;
  
  struct rsf_range_bin heights[249];
} PACKED;


struct rsf_group_512
{
  struct rsf_group_prelude prelude;
  
  struct rsf_range_bin heights[501];
} PACKED;

struct rsf_block
{
  unsigned char record_type;
  unsigned char header_length;
  unsigned char version_control;
  
  struct block_preface preface;
  
  union
  {
    struct rsf_group_128 groups_128[15];
    struct rsf_group_256 groups_256[8];
    struct rsf_group_512 groups_512[4];
  };
  
  char spare[4];
  
} PACKED;

int rsffile_bread (FILE *, int, struct rsf_block *);
int ionogram_from_rsf (struct ionogram *, FILE *);

#endif /* _RSFFILE_H */

