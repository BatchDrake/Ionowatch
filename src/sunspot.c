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
#include <math.h>

#include <sys/stat.h>
#include <unistd.h>

#include <sunspot.h>
#include <ionowatch.h>

/* TODO: define more methods for determining sunspot number */

PTR_LIST (struct sunspot_record, sunspots);

void
sunspot_add_record (int year, int month, float number)
{
  struct sunspot_record *new;
  
  new = xmalloc (sizeof (struct sunspot_record));
  
  new->year = year;
  new->month = month;
  new->number = number;
  
  PTR_LIST_APPEND (sunspots, new);
}

float
get_monthly_sunspot_number (time_t date)
{
  int i;
  
  struct tm *tm;
  
  tm = gmtime (&date);
  
  for (i = 0; i < sunspots_count; i++)
    if (sunspots_list[i]->year == tm->tm_year + 1900 &&
        sunspots_list[i]->month == tm->tm_mon + 1)
          return sunspots_list[i]->number;
          
  return NAN;
}

void
destroy_sunspot_cache (void)
{
  int i;
  
  for (i = 0; i < sunspots_count; i++)
    free (sunspots_list[i]);
    
  if (sunspots_count > 0)
    free (sunspots_list);
    
  sunspots_count = 0;
  
}

/* Be careful: if our program works for a time long enough to experience
   month changes, we have to call this repeately */
   
int
init_sunspot_cache (void)
{
  char *path;
  char line[50];
  int n, year, yr2, mon, day;
  struct tm *tm, *now;
  time_t date;
  float sunspots, sunspots_smooth;
  struct stat sbuf;
  
  FILE *fp;
  
  path = locate_config_file ("sunspot.dat", CONFIG_LOCAL | CONFIG_READ | CONFIG_WRITE | CONFIG_CREAT);
  
  if (path == NULL)
  {
    ERROR ("can't open sunspot.dat\n");
    return -1;
  }
  
  if (stat (path, &sbuf) == -1)
  {
    if (errno != ENOENT)
    {
      ERROR ("coudln't open sunspot cache: %s\n", strerror (errno));
      free (path);
      return -1; 
    }
    
    NOTICE ("no sunspot cache found, updating...\n");
    
    if (update_sunspot_cache () == -1)
    {
      free (path);
      return -1;
    }
  }
  else
  {
    time (&date);
    tm = gmtime (&sbuf.st_mtime);
    now = gmtime (&date);
    
    if (now->tm_year > tm->tm_year || 
      (now->tm_year == tm->tm_year && now->tm_mon > tm->tm_mon ))
      (void) update_sunspot_cache ();
  }
  
  if ((fp = fopen (path, "rb")) == NULL)
  {
    ERROR ("coudln't open sunspot cache: %s\n", strerror (errno));
    free (path);
    return -1;
  }
  
  while (!feof (fp))
  {
    if (fgets (line, 50, fp) != NULL)
    {
      n = sscanf (line, "%04d%02d%6d.%03d%6f%6f", 
      &year, &mon, &yr2, &day, &sunspots, &sunspots_smooth);
      
      if (n == 6)
        sunspot_add_record (year, mon, sunspots_smooth);
      else if (n == 5)
        sunspot_add_record (year, mon, sunspots);
      else
        ERROR ("line malformed in %s (%d fields got)\n", path, n);
    }
  }
  
  free (path);
  fclose (fp);
  
  return 0;
}

/* This action should be performed only once a month */
int
update_sunspot_cache (void)
{
  char *path, *new;
  int len, got;
  char buf[1024];
  FILE *fp;
  
  path = strbuild ("%s/sunspot.tmp", get_ionowatch_config_dir ());
  
  if ((fp = fopen (path, "wb")) == NULL)
  {
    ERROR ("couldn't open temporary file for storing sunspot data: %s\n",
      strerror (errno));
    free (path);
    return -1;
  }
  
  if (http_download ("http://sidc.oma.be/DATA/monthssn.dat", fp) == -1)
  {
    ERROR ("error downloading sunspot number for this month\n");
    free (path);
    fclose (fp);
    return -1;
  }
  
  fseek (fp, 0, SEEK_END);
  len = ftell (fp);
  
  if (len == 0)
  {
    ERROR ("download failed, got empty file\n");
    free (path);
    fclose (fp);
    return -1;
  }
  
  fseek (fp, 0, SEEK_SET);
  
  while ((got = fread (buf, 1, 1024, fp)) > 0)
    fwrite (buf, got, 1, fp);
  
  fclose (fp);
 
  new = locate_config_file ("sunspot.dat", CONFIG_LOCAL | CONFIG_WRITE | CONFIG_CREAT);
  
  if (rename (path, new) == -1)
  {
    ERROR ("coudln't update sunspot file: %s\n", strerror (errno));
    free (path);
    free (new);
    return -1;
  }
  
  free (path);
  free (new);
  
  return 0;
}

