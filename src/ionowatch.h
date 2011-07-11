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

#ifndef _IONOWATCH_H
#define _IONOWATCH_H

#include <util.h>

#include <fortran.h>
#include <sao.h>
#include <index.h>
#include <globe.h>
#include <sunspot.h>
#include <ionogram.h>
#include <mmmfile.h>
#include <rsffile.h>
#include <sbffile.h>
#include <config_files.h>
#include <datasource.h>
#include <plugin.h>
#include <libini.h>

/* "Aberswyth",,"AB067","-4,1000","52,4000",,"N",,,,,,"GB","UK","GBR","826" */

#define STATION_FILE_LINE_LONG_NAME 0
#define STATION_FILE_LINE_NAME      2
#define STATION_FILE_LINE_LONGITUDE 3
#define STATION_FILE_LINE_LATITUDE  4
#define STATION_FILE_LINE_REALTIME  6
#define STATION_FILE_LINE_COUNTRIES 8

#define STATION_FILE_FIELDS_MINIMUM 9

struct station_info
{
  arg_list_t *args;
  
  char *name_long;
  char *country;
  char *name;
  
  float lat, lon;
  
  int realtime;
};

int ionowatch_config_init (void);
struct strlist *parse_apache_index (const char *);

struct station_info *station_lookup (const char *);
int parse_station_file (const char *);
struct station_info *station_info_pick_rand (void);

#endif /* _IONOWATCH_H */

