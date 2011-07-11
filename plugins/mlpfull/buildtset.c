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
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <ionowatch.h>

int training = 1;

void
sigint_handler (int sig)
{
  training = 0;
}

struct strlist *
pickup_random_files ()
{
  char *url;
  char *year;
  char *yday;
  char *station;
  int i, n;
  struct strlist *tmp, *result;
  
  url = strbuild ("http://ngdc.noaa.gov/ionosonde/MIDS/data/");
  
  tmp = parse_apache_index (url);
  free (url);
  
  if (tmp->strings_count == 0)
    return tmp;
  
  n = (tmp->strings_count - 1) * ((float) rand () / (float) RAND_MAX);
  station = xstrdup (tmp->strings_list[n]);
    
  url = strbuild ("http://ngdc.noaa.gov/ionosonde/MIDS/data/%s/individual/",
    station);
  
  tmp = parse_apache_index (url);
  free (url);
  
  if (tmp->strings_count == 0)
  {
    free (station);
    return tmp;
  }
  
  n = (tmp->strings_count - 1) * ((float) rand () / (float) RAND_MAX);
  year = xstrdup (tmp->strings_list[n]);
  
  strlist_destroy (tmp);
  
  url = strbuild ("http://ngdc.noaa.gov/ionosonde/MIDS/data/%s/individual/%s/",
    station, year);
    
  
  tmp = parse_apache_index (url);
  free (url);
  
  if (tmp->strings_count == 0)
  {
    free (station);
    free (year);
    return tmp;
  }
  
  n = (tmp->strings_count - 1) * ((float) rand () / (float) RAND_MAX);
  yday = xstrdup (tmp->strings_list[n]);
   
  strlist_destroy (tmp);
  
  url = strbuild ("http://ngdc.noaa.gov/ionosonde/MIDS/data/%s/individual/%s/%s/scaled/",
    station, year, yday);
    
  free (yday);
  free (year);
  free (station);
  tmp = parse_apache_index (url);
  free (url);

  result = strlist_new ();
  
  for (i = 0; i < tmp->strings_count; i++)
  {
    if (strcmp (&tmp->strings_list[i][strlen (tmp->strings_list[i]) - 3], "SAO") == 0)
      strlist_append_string (result, tmp->strings_list[i]); /* TODO: append_STRING? that's redundant */
  }
    
  strlist_destroy (tmp);
  return result;
}

#define PATTERN_LAT         0
#define PATTERN_LON         1
#define PATTERN_INCLINATION 2
#define PATTERN_STATION     3
#define PATTERN_SUNSPOTS    4
#define PATTERN_LAYER       5

PTR_LIST (struct ionogram, training_set);

int
main (int argc, char **argv, char **envp)
{
  FILE *list, *fp;
  int i, j, count;
  int lines = 0;
  
  struct ionogram *ionogram;
  struct ionogram_filename fn;
  struct strlist *names;
  struct station_info *info;
  struct globe_data *globe;
  struct ionogram_filetype *ft;
  struct layer_info *this;
  
  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s <pattern.lst>\n", argv[0]);
    fprintf (stderr, "\n%s picks random ionogram from the NOAA server and builds\n", argv[0]);
    fprintf (stderr, "a training set with the existence state of each ionosphere layer.\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "The training set file is in ID3 format.\n");
    
    return 1;
  }
  
  signal (SIGINT, sigint_handler);
  
  if (access (argv[1], F_OK) != -1)
    fprintf (stderr, "%s: appending to existing file %s\n", argv[0], argv[1]);
  
  if ((list = fopen (argv[1], "a")) == NULL)
  {
    fprintf (stderr, "%s: couldn't append to file %s: %s\n", argv[0], argv[1], strerror (errno));
    
    return 1;
  }
  
  srand (time (NULL));
  
  if (libsao_init () == -1)
    return 1;
    
  if (ionowatch_config_init () == -1)
    return -1;
    
  globe = globe_data_new (0, 0, 100, 100);
  ft = ionogram_filetype_lookup ("SAO");
  
  printf ("%s staring (press Ctrl+C to stop)\n", argv[0]);
  
  for (; training;)
  {
    printf ("Please wait while %s looks for a suitable station...\n", argv[0]);
  
    for (;;)
    {
      names = pickup_random_files ();
      
      if (names->strings_count == 0)
      {
        strlist_destroy (names);
        continue;
      }
      
      break;
    }
    
    if (ionogram_parse_filename (names->strings_list[0], &fn) == -1)
    {
      ERROR ("%s: malformed filename, couldn't load\n", names->strings_list[i]);
      strlist_destroy (names);
      continue;
    }

    if ((info = station_lookup (fn.station)) == NULL)
    {
      ERROR ("couldn't find station data for `%s'\n", fn.station);
      strlist_destroy (names);
      continue; /* We're almost sure that there will be no other filename
                 refering to a different station in this directory */
    }
    
    for (i = 0; i < names->strings_count && training; i++)
    {
      NOTICE ("parsing files [%3d/%3d]... ", 
        i + 1, names->strings_count);
      
      fflush (stdout);
      
      if (ionogram_parse_filename (names->strings_list[i], &fn) == -1)
      {
        NOTICE ("%s: malformed filename, couldn't load\n", names->strings_list[i]);
        continue;
      }

      if ((fp = cache_get_ionogram (&fn)) != NULL)
      {
        ionogram = ionogram_new ();
        
        if ((ft->parse_callback) (ionogram, fp) == 0)
        {
          globe_data_set_time (globe, fn.time);
          
          count = 0;
          
          for (j = 0; j < IONOGRAM_MAX_LAYERS && training; j++)
          {
            fprintf (list, "%lg %lg %lg %lg %lg %d ",
              info->lat, info->lon,
              globe_data_get_sun_inclination (globe, RADADJUST (DEG2RAD (info->lat)), RADADJUST (DEG2RAD (-info->lon))),
              RADADJUST (globe->sol),
              get_monthly_sunspot_number (fn.time),
              j);
              
            if ((this = ionogram->layers[j]) != NULL)
            {
              count++;
              fprintf (list, " 1\n");
            }
            else
              fprintf (list, " 0\n");
              
            lines++;
          }
           
          printf ("%d layers present\n", count); 
        }
        else
          printf ("%s: error parsing ionogram\n", names->strings_list[i]);
        
        ionogram_destroy (ionogram);
        
        fclose (fp);
      }
      else
        NOTICE ("%s: coudln't retrieve form cache\n", names->strings_list[i]);
    }
    
    strlist_destroy (names);
  }
  
  printf ("\n%s: fetching stopped, %d samples saved to %s\n", 
    argv[0], lines, argv[1]);
    
  fclose (list);
  
  return 0;
}

