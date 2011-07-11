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


#ifndef _INDEX_H
#define _INDEX_H

#include <string.h>
#include <time.h>
#include <ionogram.h>
#include <ctype.h>

#include <util.h>

typedef unsigned long long int ionogram_bitmap_t;

struct ionogram_filename
{
  char station[6];
  int  type;
  
  int year;
  int yday;
  int h, m, s;
  
  int day, month;
  
  time_t time;
};

struct ionogram_index
{
  char *name;
  char *url;
};

struct ionogram_filetype
{
  char extension[4];
  char *desc;
  ionogram_bitmap_t   type;
  
  int  (*parse_callback) (struct ionogram *, FILE *);
  
  char *(*build_url_callback) (const struct ionogram_filename *, const char *);
};

static int
check_station_name (char *name)
{
  int i;
  
  if (strlen (name) != 5)
    return 0;
    
  for (i = 0; i < 5; i++)
    if (!isupper (name[i]) && !isdigit (name[i]))
      return 0;
      
  return 1;
}

void register_ionogram_inventory (const char *, const char *);
FILE *cache_get_ionogram (struct ionogram_filename *);
int ionogram_parse_filename (const char *, struct ionogram_filename *);
int move_file_data (FILE *, FILE *);
char *ionogram_filename_str (struct ionogram_filename *);
struct strlist *get_day_ionograms (const struct ionogram_filename *);
struct ionogram_filetype *ionogram_filetype_lookup (const char *);


#endif /* _INDEX_H */

