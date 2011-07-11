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

#include "trainer.h"

double autotrain_max_load = .1;

static struct mlp *h2pf_mlp;
static struct mlp *predictor_mlp;
static struct mlp *plasma_mlp;

static int
load_weights_from_config (struct mlp *mlp, const char *file)
{
  char *weightfile;
  
  if ((weightfile = locate_config_file (file, CONFIG_READ | CONFIG_LOCAL | CONFIG_GLOBAL)) == NULL)
  {
    ERROR ("unable to open MLP %s, no weights loaded\n", file);
    return -1;
  }
  if (mlp_load_weights (mlp, weightfile) == -1)
  {
    ERROR ("invalid MLP file %s\n", file);
    free (weightfile);
    return -1;
  }
  
  free (weightfile);
  
  return 0;
}

int
save_weights (struct mlp *mlp, const char *file)
{
  char *weightfile;
  char *weightfile_tmp;
  char *tmpfile;
  
  int fd;
  
  tmpfile = strbuild ("%s/weightsXXXXXX", get_ionowatch_config_dir ());
  
  if ((fd = mkstemp (tmpfile)) == -1)
  {
    ERROR ("unable to open tmp file: %s\n", strerror (errno));
    free (tmpfile);
    return -1;
  }
  
  close (fd);
  
  weightfile = strbuild ("%s/%s", get_ionowatch_config_dir (), file);
  weightfile_tmp = strbuild ("%s/%s.%d.%d", 
    get_ionowatch_config_dir (), file, time (NULL), getpid ());
  
  if (mlp_save_weights (mlp, tmpfile) == -1)
  {
    ERROR ("couldn't save weights to %s\n", file);
    free (weightfile);
    free (weightfile_tmp);
    
    free (tmpfile);
    return -1;
  }
  
  if (rename (weightfile, weightfile_tmp) == -1)
  {
    if (errno != ENOENT)
    {
      ERROR ("unespected error moving files, weights saved in %s\n", tmpfile);
      free (weightfile);
      free (weightfile_tmp);
      
      free (tmpfile);
      return -1;
    }
  }
  
  if (rename (tmpfile, weightfile) == -1)
  {
    ERROR ("unespected error moving files, weights saved in %s\n", tmpfile);
    free (weightfile);
    free (weightfile_tmp);
    
    free (tmpfile);
    return -1;
  }
  
  unlink (weightfile_tmp);
  
  free (weightfile);
  free (weightfile_tmp);
    
  free (tmpfile);
    
  return 0;
}

void
sleep_until_low_load (void)
{
  double average;
  
  while (getloadavg (&average, 1), average > autotrain_max_load)
    sleep (60);
}

void *
trainer_thread_entry (void *unused)
{
  struct training_set *set;
  
  struct strlist *names;
  struct ionogram_filename fn;
  struct station_info *info;
  struct ionogram *ionogram;
  struct ionogram_filetype *ft;
  struct globe_data *globe;
  
  FILE *fp;
  
  double average;
  int i;
  
  if ((ft = ionogram_filetype_lookup ("SAO")) == NULL)
  {
    ERROR ("no SAO filetype registered\n");
    return NULL;
  }
  
  h2pf_mlp      = h2pf_build_mlp ();
  predictor_mlp = predictor_build_mlp ();
  plasma_mlp    = plasma_build_mlp ();
  
  (void) load_weights_from_config (h2pf_mlp, "h2pf.mlp");
  (void) load_weights_from_config (plasma_mlp, "plasma.mlp");
  (void) load_weights_from_config (predictor_mlp, "predictor.mlp");
  
  globe = globe_data_new (0, 0, 0, 0);
  
  set = training_set_new ();

  for (;;)
  {
    getloadavg (&average, 1);
    
    DEBUG ("load average: %lg\n", average);
    
    if (average >= autotrain_max_load)
    {
      DEBUG ("load average (%lg) too high to train, 15 minutes to restart\n", average);
      sleep (900);
      continue;
    }
    
    DEBUG ("system load is ok to train, (%lg <= %lg) starting...\n", average, autotrain_max_load);

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
        DEBUG ("%s: malformed filename, couldn't load\n", names->strings_list[0]);
        strlist_destroy (names);
        continue;
      }

      if ((info = station_lookup (fn.station)) == NULL)
      {
        DEBUG ("couldn't find station data for `%s'\n", fn.station);
        strlist_destroy (names);
        continue;
      }
      
      break;
    }
    
    if (!names->strings_count)
    {
      DEBUG ("no ionograms this day\n");
      strlist_destroy (names);
      continue;
    }
    
    sleep_until_low_load ();
      
    for (i = 0; i < names->strings_count; i++)
    {
      DEBUG ("parsing files [%3d/%3d]...\n", 
        i + 1, names->strings_count);
      
      if (ionogram_parse_filename (names->strings_list[i], &fn) == -1)
      {
        DEBUG ("%s: malformed filename, couldn't load\n", names->strings_list[i]);
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
          DEBUG ("%s: error parsing ionogram\n", names->strings_list[i]);
          
        fclose (fp);
        
      }
      else
        DEBUG ("%s: coudln't retrieve form cache\n", names->strings_list[i]);
    }
    
    sleep_until_low_load ();
    
    
    training_set_set_epoch_count (set, 1000);
    training_set_set_info_interval (set, 0);
    training_set_set_training_speed (set, 1e-6);
  
    DEBUG ("Start layer MLP training...\n");
    
    /* Build training set for layers */
    predictor_training_set_build (set);
    
    training_set_train_on_mlp (set, predictor_mlp);
    
    training_set_destroy_vectors (set);
    
    save_weights (predictor_mlp, "predictor.mlp");
    
    
    sleep_until_low_load ();
    
    training_set_set_training_speed (set, 1e-5);
    
    DEBUG ("Start plasma MLP training...\n");
    
    /* Build training set for layers */
    plasma_training_set_build (set);
    
    training_set_train_on_mlp (set, plasma_mlp);
    
    training_set_destroy_vectors (set);
    
    save_weights (plasma_mlp, "plasma.mlp");
    
    sleep_until_low_load ();
    
    
    training_set_set_training_speed (set, 1e-7);
    
    DEBUG ("Start height to plasma MLP training...\n");
    
    /* Build training set for layers */
    h2pf_training_set_build (set);
    
    training_set_train_on_mlp (set, h2pf_mlp);
    
    training_set_destroy_vectors (set);
    
    save_weights (h2pf_mlp, "h2pf.mlp");
    
    training_set_destroy (set);
    
    set = training_set_new ();    
  }
}

