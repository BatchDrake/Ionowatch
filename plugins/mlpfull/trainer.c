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
#include <sys/time.h>

#include <pthread.h>

#include "trainer.h"

struct strlist *
pickup_random_files (void)
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

struct training_set *
training_set_new (void)
{
  struct training_set *new;
  
  new = xmalloc (sizeof (struct training_set));
  
  memset (new, 0, sizeof (struct training_set));
  
  new->epoch_count = DEFAULT_EPOCH_COUNT;
  new->training = 1;
  new->info_interval = DEFAULT_INFO_INTERVAL;
  new->desired_mse = DEFAULT_DESIRED_MSE;
  new->training_speed = DEFAULT_MLP_TRAINING_SPEED;
  return new;
}

void
training_set_add_ionogram (struct training_set *set,
                           struct ionogram *ionogram)
{
  PTR_LIST_APPEND (set->training_set, ionogram);
}

void
training_set_add_vectors (struct training_set *set,
                          numeric_t *input,
                          numeric_t *output)
{
  PTR_LIST_APPEND (set->training_vector, input);
  PTR_LIST_APPEND (set->desired_output, output);
}

void
training_set_set_epoch_count (struct training_set *set, int val)
{
  set->epoch_count = val;
}

void
training_set_set_info_interval (struct training_set *set, int val)
{
  set->info_interval = val;
}

void
training_set_set_desired_mse (struct training_set *set, numeric_t val)
{
  set->desired_mse = val;
}

void
training_set_set_training_speed (struct training_set *set, numeric_t val)
{
  set->training_speed = val;
}

void
training_set_destroy_vectors (struct training_set *set)
{
  int i;
    
  for (i = 0; i < set->training_vector_count; i++)
    if (set->training_vector_list[i] != NULL)
      free (set->training_vector_list[i]);
      
  if (set->training_vector_list != NULL)
    free (set->training_vector_list);
    
  set->training_vector_list = NULL;
  set->training_vector_count = 0;
  
  for (i = 0; i < set->desired_output_count; i++)
    if (set->desired_output_list[i] != NULL)
      free (set->desired_output_list[i]);
      
  if (set->desired_output_list != NULL)
    free (set->desired_output_list);
    
  set->desired_output_list = NULL;
  set->desired_output_count = 0;
}

void
training_set_destroy (struct training_set *set)
{
  int i;
  
  for (i = 0; i < set->training_set_count; i++)
    if (set->training_set_list[i] != NULL)
      ionogram_destroy (set->training_set_list[i]);
      
  if (set->training_set_list != NULL)
    free (set->training_set_list);
    
  for (i = 0; i < set->training_vector_count; i++)
    if (set->training_vector_list[i] != NULL)
      free (set->training_vector_list[i]);
      
  if (set->training_vector_list != NULL)
    free (set->training_vector_list);
    
  for (i = 0; i < set->desired_output_count; i++)
    if (set->desired_output_list[i] != NULL)
      free (set->desired_output_list[i]);
      
  if (set->desired_output_list != NULL)
    free (set->desired_output_list);
    
  free (set);
}


/* Ugly hack */
static __thread struct training_set *current;



#ifdef USE_LIBFANN

static void
load_vector (unsigned int i, 
             unsigned int input_size, 
             unsigned int output_size, 
             fann_type *input_target,
             fann_type *output_target)
{
  memcpy (input_target, current->training_vector_list[i], current->input_len * sizeof (numeric_t));
  memcpy (output_target, current->desired_output_list[i], current->output_len * sizeof (numeric_t));
}

numeric_t
training_set_train_on_fann (struct training_set *set,
                           struct fann *ann)
{
  struct fann_train_data *data;
  
  current = set;
  
  fann_set_activation_function_hidden (ann, FANN_SIGMOID);
	fann_set_activation_function_output (ann, FANN_LINEAR);
	
	fann_set_training_algorithm (ann, FANN_TRAIN_INCREMENTAL);
	
  NOTICE ("FANN: loading vectors...\n");
  
  data = fann_create_train_from_callback (set->training_vector_count,
                                          set->input_len,
                                          set->output_len,
                                          load_vector);
                            
  NOTICE ("FANN:   learning_rate: %lg\n", ann->learning_rate);
  NOTICE ("FANN:   epsilon: %lg\n", ann->learning_rate / (fann_type) set->training_vector_count);
  NOTICE ("FANN:   learning_momentum: %lg\n", ann->learning_momentum);
  
  NOTICE ("FANN: load done, training...\n");
  
  fann_train_on_data (ann, data, set->epoch_count, set->info_interval, set->desired_mse);
  
  fann_destroy_train (data);
  
  current = NULL;
  
  return fann_get_MSE (ann);
}

#else

static void
sigint_handler (int sig)
{
  printf ("SIGINT: %p\n", current);
  
  current->training = 0;
}

numeric_t 
training_set_train_on_mlp (struct training_set *set,
                           struct mlp *mlp)
{
  int n, i, j;
  int info;
  
  void (*saved) (int);
  
  numeric_t mse;
  numeric_t best_mse;
  numeric_t *output;
  struct timeval tv, otv;
  
  best_mse = INFINITY;
  
  current = set;
  
  saved = signal (SIGINT, sigint_handler);
  fprintf (stderr, "Training start with eta=%lg\n", set->training_speed);
  fprintf (stderr, "Epoch count: %d, training: %d\n", 
    set->epoch_count, set->training);
  
  gettimeofday (&otv, NULL);
  
  for (n = 0; n < set->epoch_count && set->training; n++)
  {
    mse = 0.0;
    info = set->info_interval ? ((n + 1) % set->info_interval) == 0 || n == 0 : 0;
    
    for (i = 0; i < set->training_vector_count;  i ++)
    {  
      output = mlp_eval (mlp, set->training_vector_list[i]);
      
      mlp_propagate_error (mlp, set->desired_output_list[i]);
      mse += mlp->current_mse;
        
      mlp_update_weights (mlp, set->training_speed);
     
      if (info && set->debug_vector)
      {
        fprintf (stderr, "\noutput[%d] = {", set->output_len);
        
        for (j = 0; j < set->output_len; j++)
        {
          if (j)
            fprintf (stderr, ", ");
          fprintf (stderr, "%.5lf", output[j]);
        }
        
        fprintf (stderr, "};\n");
        
        fprintf (stderr, "target[%d] = {", set->output_len);
        
        
        for (j = 0; j < set->output_len; j++)
        {
          if (j)
            fprintf (stderr, ", ");
            
          fprintf (stderr, "%.5lf", set->desired_output_list[i][j]);
        }
        
        fprintf (stderr, "};\n");
        
      }
    }
    
    
    mse /= set->training_vector_count;
    
    if (mse < best_mse)
      best_mse = mse;
      
    mlp->current_mse = mse;
    mlp->best_mse = best_mse;
    
    if (mse < set->desired_mse) /* Stop training */
    {
      fprintf (stderr, "better MSE reached (%g < %g at epoch %d)\n", mse, set->desired_mse, n + 1);
      set->training = 0;
    }
          
    if (info)
    {
      gettimeofday (&tv, NULL);
      otv.tv_sec = tv.tv_sec - otv.tv_sec;
      otv.tv_usec = tv.tv_usec - otv.tv_usec;
      
      if (otv.tv_usec < 0)
      {
        otv.tv_usec += 1000000;
        otv.tv_sec++;
      }
      
      fprintf (stderr, "epoch %5d: MSE %.20f (%d vectors) dt=%5d.%06d\n", 
        n + 1, mse, set->training_vector_count, (int) otv.tv_sec, (int) otv.tv_usec);
        
      otv = tv;
    }
  }
  
  set->passed_epochs = n;
  
  signal (SIGINT, saved);
  current = NULL;
  
  return best_mse;
}

#endif

