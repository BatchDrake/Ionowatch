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
#include <sbffile.h>

#include <ionogram.h>

int
sbffile_bread (FILE *fp, int blocknr, struct sbf_block *block)
{
  if (block < 0)
  {
    ERROR ("negative block requested (very likely to be a bug)\n");
    return 0;
  }
  
  fseek (fp, blocknr * sizeof (struct sbf_block), SEEK_SET);
  
  errno = 0;
  
  if (fread (block, sizeof (struct sbf_block), 1, fp) < 1)
    return 0;
  
  if ((block->record_type != RECORD_TYPE_SBF_1ST && block == 0) &&
      (block->record_type != RECORD_TYPE_SBF     && block != 0))
  {
    ERROR ("invalid block read, invalid SBF file (record type %02x unknown)\n",
      block->record_type);
    return -1;
  }
  
  // block_preface_debug (&block->preface);
  return 1;
}


