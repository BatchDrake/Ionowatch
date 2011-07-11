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

#include "trainer.h"

extern char *default_weight_file;

void
help (char **argv)
{
  fprintf (stderr, "%s: neural network trainer for ionowatch\n", argv[0]);
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "\t%s [OPTS] files...\n", argv[0]);
  fprintf (stderr, "\n");
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "\t-n <num epochs>  Specifies the max. number of epochs to iterate\n");
  fprintf (stderr, "\t-e <error>       Specifies the error goal to achieve\n");
  fprintf (stderr, "\t-w <file>        Specifies the weight file to save data\n");
  fprintf (stderr, "\t-I <interval>    Epoch interval between messages\n");
  fprintf (stderr, "\t-s <eta>        Training speed parameter\n");
  fprintf (stderr, "\t-i               Interrogate before replacing weight file\n");
  fprintf (stderr, "\t-b               Save only if error goal is reached\n");
  fprintf (stderr, "\t-h               This help\n");
  
}

int
main (int argc, char **argv)
{
  FILE *fp;
 
#ifdef USE_LIBFANN
  struct fann *ann;
#else
  struct mlp *mlp;
#endif
  struct training_set *set;
  
  struct ionogram *ionogram;
  struct ionogram_filename fn;
  struct strlist *names, *files;
  struct station_info *info;
  struct globe_data *globe;
  struct ionogram_filetype *ft;
  
  char c;
  
  numeric_t best_mse, best_mse_before;
  
  int i;
  
  char *weightfile = default_weight_file;
  int interrogate = 0;
  int weightfile_flag = 0;
  int best_flag = 0;
  
  files = strlist_new ();

  set = training_set_new ();
  
  
  while ((c = getopt (argc, argv, ":n:e:w:s:I:ibh")) != -1)
  {
    switch (c)
    {
      case 'n':
        if (!sscanf (optarg, "%i", &set->epoch_count))
        {
          fprintf (stderr, "%s: option -n expects a number\n", argv[0]);
          return 1;
        }
        break;
      
      case 'I':
        if (!sscanf (optarg, "%i", &set->info_interval))
        {
          fprintf (stderr, "%s: option -I expects a number\n", argv[0]);
          return 1;
        }
        
        if (set->info_interval < 1)
        {
          fprintf (stderr, "%s: interval must be strictly positive\n", argv[0]);
          return 1;
        }
        
        break;
      
      case 'b':
        best_flag++;
        break;
        
        
      case 'e':
        if (!sscanf (optarg, "%lg", &set->desired_mse))
        {
          fprintf (stderr, "%s: option -e expects a number\n", argv[0]);
          return 1;
        }
        break;
      
      case 's':
        if (!sscanf (optarg, "%lg", &set->training_speed))
        {
          fprintf (stderr, "%s: option -s expects a number\n", argv[0]);
          return 1;
        }
        break;
      
      case 'w':
        weightfile = optarg;
        weightfile_flag++;
        break;
        
      case 'i':
        interrogate++;
        break;
        
      case ':':
        fprintf (stderr, "%s: option -%c requires an argument\n", argv[0], optopt);
        help (argv);
        exit (1);
        break;
        
      case '?':
        fprintf (stderr, "%s: unrecognized option -- -%c\n", argv[0], optopt);
        help (argv);
        exit (1);
        break;
        
      case 'h':
        help (argv);
        exit (0);
        break; /* Sure, sure */
        
    }
  }
  
  for (i = optind; i < argc; i++)
    strlist_append_string (files, argv[i]);
    
  srand (time (NULL));
  
  if (libsao_init () == -1)
    return 1;
    
  if (ionowatch_config_init () == -1)
    return -1;

#ifdef USE_LIBFANN
  ann = build_fann ();
  
  best_mse = INFINITY;
  best_mse_before = INFINITY;
  
#else
  mlp = build_mlp ();
  
  NOTICE ("trying to load weights from %s...\n", weightfile);
  
  if (mlp_load_weights (mlp, weightfile) == -1)
    NOTICE ("failed: %s\n", strerror (errno));
  else
    NOTICE ("done\n");
  
  best_mse_before = mlp->best_mse;
#endif

  printf ("Best MSE: %g\n", best_mse_before);
  
  globe = globe_data_new (0, 0, 0, 0);
  ft = ionogram_filetype_lookup ("SAO");
  
  info = NULL;
  names = NULL;
  
  if (files->strings_count > 0)
  {
    if (files->strings_count > 1)
    {
      ERROR ("currently one file at a time supported\n");
      return 1; /* This is because I'm too lazy to save pointers to stations */
    }
    
    for (i = 0; i < files->strings_count; i++)
    {
      if (ionogram_parse_filename (files->strings_list[i], &fn) == -1)
      {
        NOTICE ("%s: malformed filename, couldn't load\n", files->strings_list[i]);
        continue;
      }
    
      if (fn.type != ft->type)
      {
        ERROR ("not a SAO file, only SAO files supported\n");
        return 1;
      }
      
      if ((info = station_lookup (fn.station)) == NULL)
      {
        ERROR ("couldn't find station data for `%s'\n", fn.station);
        strlist_destroy (names);
        return 1; /* We're almost sure that there will be no other filename
                   refering to a different station in this directory */
      }
        
      names = get_day_ionograms (&fn);
    }
  }
  else
  {
    NOTICE ("looking for a suitable station...\n");

    for (;;)
    {
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
        ERROR ("%s: malformed filename, couldn't load\n", names->strings_list[0]);
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
      
      break;
    }
  }
  
  if (!names->strings_count)
  {
    ERROR ("no ionograms that day!\n");
    return 1;
  }
  
  printf ("file: %s\n", names->strings_list[0]);
  
  NOTICE ("configutarion is: %d epochs, taking %g as minimum MSE "
          "warning every %d epochs\n", set->epoch_count, set->desired_mse, set->info_interval);
  if (best_flag)
    NOTICE ("weights will be saved ONLY if they achieve better results\n");
  
  
  
  NOTICE ("selected station %s (%s, %s)\n", 
    fn.station, info->name_long, info->country);
  NOTICE ("daily files are of the form %s\n", names->strings_list[0]);
  
  NOTICE ("ionotrainer is going to parse %d files, please wait...\n",
    names->strings_count);
  
  for (i = 0; i < names->strings_count; i++)
  {
    NOTICE ("parsing files [%3d/%3d]... ", 
      i + 1, names->strings_count);
    
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
        ionogram->lat = RADADJUST (DEG2RAD (info->lat));
        ionogram->lon = RADADJUST (DEG2RAD (-info->lon));
        globe_data_set_time (globe, fn.time);
        
        ionogram->sun_inclination = globe_data_get_sun_inclination (globe, ionogram->lat, ionogram->lon);

        ionogram->solstice_offset = RADADJUST (globe->sol);
        ionogram->sunspot_number = get_monthly_sunspot_number (fn.time);
        
        training_set_add_ionogram (set, ionogram);
      }
      else
        NOTICE ("%s: error parsing ionogram\n", names->strings_list[i]);
        
      fclose (fp);
      
      printf ("\n");
      
    }
    else
      NOTICE ("%s: coudln't retrieve from cache\n", names->strings_list[i]);
  }
  
  NOTICE ("work done!\n");
  
  NOTICE ("preparing training vectors...\n");
  
  training_set_build (set);
  
  NOTICE ("got %d training vectors\n", set->training_vector_count);

  if (!set->training_vector_count)
  {
    fprintf (stderr, "no training vectors, exiting...\n");
    return 0;
  }
  
#ifdef USE_LIBFANN
  best_mse = training_set_train_on_fann (set, ann);
#else
  best_mse = training_set_train_on_mlp (set, mlp);
#endif

  if (best_mse < best_mse_before)
    fprintf (stderr, "training ended with better MSE at %d epochs (mse: %g)\n", set->passed_epochs, best_mse);
  else
    fprintf (stderr, "Training stopped, no better MSE found\n");
    
#ifdef USE_LIBFANN
  fprintf (stderr, "Weights NOT saved\n");
  
#else
  if (!best_flag || best_mse < best_mse_before)
  {
    if (interrogate)
    {
      fprintf (stderr, "do you want to save weights to %s? [y/N] ", weightfile);
      c = getchar ();
    }
    
    if (c == 'y' || c == 'Y' || !interrogate)
    {
      if (mlp_save_weights (mlp, weightfile) == -1)
        fprintf (stderr, "saving failed: %s\n", strerror (errno));
      else
        fprintf (stderr, "weights saved\n");
    }
    else
      fprintf (stderr, "not saved\n");
  }
#endif
  return 0;
}

