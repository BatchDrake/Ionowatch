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

#include <ionowatch.h>


void
ionogram_to_matlab (FILE *fp, struct ionogram *ionogram, int n)
{
  int i, j;
  
  for (j = 0; j < IONOGRAM_MAX_LAYERS; j++)
    if (ionogram->layers[j] != NULL)
    {
      for (i = 0; i < ionogram->layers[j]->point_count; i++)
      {
        fprintf (fp, "heights(%d, %d, %d) = %g; ", ionogram->layers[j]->type + 1, n + 1, i + 1, ionogram->layers[j]->heights[i]);
        fprintf (fp, "freqs(%d, %d, %d) = %g;\n", ionogram->layers[j]->type + 1, n + 1, i + 1, ionogram->layers[j]->frequencies[i]);
      } 
    }
}

int
main (int argc, char **argv)
{
  FILE *fp;
  int i;
  
  struct strlist *list;
  struct ionogram_filename fn;
  struct ionogram_filetype *ft;
  struct ionogram *ionogram;
  
  if (argc != 2)
  {
    fprintf (stderr, "Useform:\n");
    fprintf (stderr, "\t%s FILE.EXT\n", argv[0]);
    fprintf (stderr, "\nNote the file name must have the right format\n");
    
    exit (1);
  }
  
  if (libsao_init () == -1)
    return 1;
    
  if (ionowatch_config_init () == -1)
    return 1;
    
  
  ft = ionogram_filetype_lookup ("SAO");
  
  if (ionogram_parse_filename (argv[1], &fn) == -1)
  {
    ERROR ("malformed filename, couldn't load\n");
    return 1;
  }
  
  if (fn.type != ft->type)
  {
    ERROR ("not a SAO file, only SAO files supported\n");
    return 1;
  }
  
  list = get_day_ionograms (&fn);
  
  printf ("heights = zeros (1, 1, 1);\n");
  printf ("freqs = zeros (1, 1, 1);\n");
  
  for (i = 0; i < list->strings_count; i++)
  {
    if (list->strings_list[i] != NULL)
    {
      if (ionogram_parse_filename (list->strings_list[i], &fn) == -1)
      {
        ERROR ("%s: malformed filename, couldn't load\n", list->strings_list[i]);
        continue;
      }
      
      ionogram = ionogram_new ();
      
      if ((fp = cache_get_ionogram (&fn)) != NULL)
      {
        (ft->parse_callback) (ionogram, fp);
        fclose (fp);
      }
      else
      {
        ERROR ("%s: coudln't retrieve form cache\n", list->strings_list[i]);
        ionogram_destroy (ionogram);
        continue;
      }
      
      ionogram_to_matlab (stdout, ionogram, i);
      
      ionogram_destroy (ionogram);
    }
  }
  
  return 0;
}

