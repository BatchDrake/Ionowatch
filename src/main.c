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

#include <libsim.h>

#define DEFAULT_FGCOLOR 0xffffa500
#define DEFAULT_BGCOLOR 0xff000000

#define DEFAULT_PLASMA_COLOR 0xff00ffff

int
main (int argc, char **argv)
{
  display_t *disp;
  char *layer_names[] = {"F2 (o)", "F2 (x)", "F1 (o)", "F1 (x)", 
                          "E (o)", "E (x)", "Es (o)", "Es(x)", 
                          "E Auroral (o)", "E Auroral (x)"};
  
  int last_x, last_y;
  int x;
  
  int i, j, empty = 0;
  int first = 0;
  float freq, height;
  
  struct ionogram_filetype *ft;
  struct ionogram_filename fn;
  struct station_info *info;
  struct ionogram *ionogram;
  struct rsf_block block;
  struct globe_data *globe;
  struct tm *tm;
  
  FILE *fp;
  
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
    
  /* BC840_2010250000005.MMM */
  /* Let's extract some information out of the filename */
  if (ionogram_parse_filename (argv[1], &fn) == -1)
  {
    ERROR ("malformed filename, couldn't load\n");
    return 1;
  }
  
  /* Lookup where this station is located */
  if ((info = station_lookup (fn.station)) == NULL)
  {
    printf ("Couldn't find station data for `%s'\n", fn.station);
    return 1;
  }
  
  ionogram = ionogram_new ();
  
  /* TODO: get_ionogram? This should be named open_ionogram */
  
  ft = ionogram_filetype_lookup ("SAO");
  fn.type = ft->type;
  
  if ((fp = cache_get_ionogram (&fn)) != NULL)
  {
    (ft->parse_callback) (ionogram, fp);
    fclose (fp);
  }
  else
  {
    ERROR ("cannot continue without SAO data\n");
    return 1;
  }
  
  fp = NULL;
  
  ft = ionogram_filetype_lookup ("MMM");
  fn.type = ft->type;
  
  if ((fp = cache_get_ionogram (&fn)) == NULL)
  {
    ft = ionogram_filetype_lookup ("RSF");
    fn.type = ft->type;
    
    if ((fp = cache_get_ionogram (&fn)) == NULL)
    {
      ft = ionogram_filetype_lookup ("SBF");
      fn.type = ft->type;
      
      fp = cache_get_ionogram (&fn);
    }
  } 
 
  if (fp != NULL)
  { 
    (ft->parse_callback) (ionogram, fp);
    fclose (fp);
  }
  else
  {
    empty = 1;
    ionogram_setup_extended (ionogram, 512, 400, 1.0, 16.0, 60.0, 700.0);
  }
  
  /* Let's paint what we've got */
  if ((disp = display_new (640, 480)) == NULL)
    return 1;
    
  display_printf (disp, 0, 0, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
    "IonoWatch 0.1");
  
  globe = globe_data_new (0, 0, 100, 100);
  globe_data_set_time (globe, fn.time);
  
  display_printf (disp, 0, 8, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
    "Ionogram made by %s in %s (%s): %2.2f N, %2.2f E",
      info->name, info->name_long, info->country, info->lat, info->lon);
  
  
  display_printf (disp, 0, 16, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
    "Ionogram generated at %s", ctime (&fn.time));

  display_printf (disp, 0, 24, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
    "Sun inclination: %lg\xf8, sunspot number that month: %g", 
      globe_data_get_sun_inclination (globe, DEG2RAD (info->lat), DEG2RAD (-info->lon)),
      get_monthly_sunspot_number (fn.time));
  
  if (!empty)
  {    
    display_printf (disp, 0, 32, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
      "Raw version (as recorded by ionogram)");
        
     
    for (i = 0; i < ionogram->freq_count; i++)
      for (j = 0; j < ionogram->height_count; j++)
      {
        pset_abs (disp, i, 40 + ionogram->height_count - j - 1, 
          OPAQUE (RGB ((int) (ionogram->o_scans[i][j] * 255.0),
                       (int) (ionogram->o_scans[i][j] * 255.0),
                       (int) (ionogram->o_scans[i][j] * 255.0))));

      }
    
    
    
    ionogram_normalize_dr (ionogram);
    
    display_printf (disp, ionogram->freq_count + 8, 32, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
      "Normalized version");
        
     
    for (i = 0; i < ionogram->freq_count; i++)
      for (j = 0; j < ionogram->height_count; j++)
      {
        pset_abs (disp, ionogram->freq_count + 8 + i, 40 + ionogram->height_count - j - 1, 
          OPAQUE (RGB ((int) (ionogram->o_scans[i][j] * 255.0),
                       (int) (ionogram->o_scans[i][j] * 255.0),
                       (int) (ionogram->o_scans[i][j] * 255.0))));
      }
      
    box (disp, ionogram->freq_count + 8, 40, 
           2 * ionogram->freq_count + 8, 40 + ionogram->height_count, DEFAULT_FGCOLOR);
             
  }
  else
    display_printf (disp, 0, 32, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
      "SAO ionogram");
      
  box (disp, 0, 40, ionogram->freq_count, 40 + ionogram->height_count, DEFAULT_FGCOLOR);
  
  
  
  display_printf (disp, 0, 48 + ionogram->height_count, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
    "Ionogram resolution parameters:");
  
  display_printf (disp, 0, 56 + ionogram->height_count , DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
    "Height info: %d height samples per frequency, ranging from %g km to %g km",
    ionogram->height_count, ionogram->height_start, ionogram->height_stop
    );
  
  display_printf (disp, 0, 64 + ionogram->height_count , DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
    "Frequency info: %d frequencies measured, ranging from %g MHz to %g MHz",
    ionogram->freq_count, ionogram->freq_start, ionogram->freq_stop
    );
  
  first = 1;
      
  for (i = 0; i < ionogram->freq_count; i++)
  {
    height = ionogram_plasma_index_to_height (ionogram, i);
    
    freq = ionogram_interpolate_plasma_freq (ionogram, height);
    
    if (isnan (freq))
    {
      first = 1;
      continue;
    }
    
    x = (float) (ionogram->freq_count - 1) * (freq - ionogram->freq_start) / (ionogram->freq_stop - ionogram->freq_start);
    
    height -= ionogram->height_start;
    height /= (ionogram->height_stop - ionogram->height_start);
    height *= (ionogram->height_count - 1);
    
    
    if (first)
      pset_abs (disp, x, 40 + ionogram->height_count - height, DEFAULT_PLASMA_COLOR);
    else
      line (disp, last_x, 40 + ionogram->height_count - last_y, 
                  x, 40 + ionogram->height_count - height, DEFAULT_PLASMA_COLOR);
    
    if (!empty)
    {
      if (first)
        pset_abs (disp, ionogram->freq_count + 8 + x, 40 + ionogram->height_count - height, DEFAULT_PLASMA_COLOR);
      else
        line (disp, ionogram->freq_count + 8 + last_x, 40 + ionogram->height_count - last_y, 
                  ionogram->freq_count + 8 + x, 40 + ionogram->height_count - height, DEFAULT_PLASMA_COLOR);
    }
    
    last_x = x;
    last_y = height;
    
    first = 0;
  }
  
  first = 1;
  
  for (j = 0; j < IONOGRAM_MAX_LAYERS; j++)
    if (ionogram->layers[j] != NULL)
    {
       display_printf (disp, 0, 72 + ionogram->height_count + j * 8, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR,
         "Layer recorded: %g MHz to %g MHz (%s)",
          ionogram->layers[j]->frequencies[0],
          ionogram->layers[j]->frequencies[ionogram->layers[j]->point_count - 1],
          layer_names[j]);
        
      first = 1;
      
      for (i = 0; i < ionogram->freq_count; i++)
      {
        freq = ionogram_index_to_freq (ionogram, i);
        
        if (freq < ionogram->layers[j]->frequencies[0] || 
            freq > ionogram->layers[j]->frequencies[ionogram->layers[j]->point_count - 1])
        {
          first = 1;
          continue;
        }
        
        height = layer_interpolate_height (ionogram->layers[j], freq);
        
        height -= ionogram->height_start;
        height /= (ionogram->height_stop - ionogram->height_start);
        height *= (ionogram->height_count - 1);
        
        if (first)
          pset_abs (disp, i, 40 + ionogram->height_count - height, DEFAULT_FGCOLOR);
        else
          line (disp, last_x, 40 + ionogram->height_count - last_y, 
                      i, 40 + ionogram->height_count - height, DEFAULT_FGCOLOR);
        
        if (!empty)
        {
          if (first)
            pset_abs (disp, ionogram->freq_count + 8 + i, 40 + ionogram->height_count - height, DEFAULT_FGCOLOR);
          else
            line (disp, ionogram->freq_count + 8 + last_x, 40 + ionogram->height_count - last_y, 
                      ionogram->freq_count + 8 + i, 40 + ionogram->height_count - height, DEFAULT_FGCOLOR);
        }
        
        last_x = i;
        last_y = height;
        
        first = 0;
      }
    }     
  display_end (disp);
  
  return 0;
}

