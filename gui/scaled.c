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
#include <pthread.h>

#include <ionowatch.h>

#include "gui.h"

/* No, seriously, need an interface for managing threads */
/* TODO: define a maximum number of ionograms cached */
struct scaled_data_cache
{
  PTR_LIST (struct ionogram, cached_ionogram);
  pthread_mutex_t  mutex;
};

struct fetcher_thread_data
{
  char *path;
  pthread_t thread;
};

/* TODO: create a mutexed PTR_LIST macro */
PTR_LIST (struct fetcher_thread_data, fetcher_thread);
pthread_mutex_t fetcher_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

void ionowatch_queue_ionogram (const char *);

struct datasource *scaled_datasource;

struct datasource_magnitude *virtual_f2_height;
struct datasource_magnitude *virtual_f1_height;
struct datasource_magnitude *virtual_e_height;
struct datasource_magnitude *virtual_es_height;
struct datasource_magnitude *virtual_ea_height;
struct datasource_magnitude *plasma_frequency;
struct datasource_magnitude *plasma_peak_height;
struct datasource_magnitude *muf;
struct datasource_magnitude *tec;

struct ionogram_filetype *sao_ft;

static void
cache_lock (struct scaled_data_cache *cache)
{
  pthread_mutex_lock (&cache->mutex);
}

static void
cache_unlock (struct scaled_data_cache *cache)
{
  pthread_mutex_unlock (&cache->mutex);
}

static struct ionogram *
unsafe_cache_lookup_ionogram (struct scaled_data_cache *cache,
                              struct station_info *station,
                              time_t date, int time_diff_max)
{
  int i;
  int best_fit, best_fit_id;

  best_fit_id = -1;
  best_fit    = time_diff_max + 1;
  
  for (i = 0; i < cache->cached_ionogram_count; i++)
    if (cache->cached_ionogram_list[i] != NULL)
    {
      if (cache->cached_ionogram_list[i]->time <= date &&
          cache->cached_ionogram_list[i]->station == station)
      {
        if ((date - cache->cached_ionogram_list[i]->time) < best_fit)
        {
          best_fit_id = i;
          best_fit    = date - cache->cached_ionogram_list[i]->time;
        }
      }
    }
        
  if (best_fit_id != -1)
    return cache->cached_ionogram_list[best_fit_id];
    
  return NULL;
}

static struct ionogram *
cache_lookup_ionogram (struct scaled_data_cache *cache,
                              struct station_info *station,
                              time_t date, int time_diff_max)
{
  struct ionogram *result;
  
  cache_lock (cache);
  
  result = unsafe_cache_lookup_ionogram (cache, station, date, time_diff_max);
  
  cache_unlock (cache);
  
  return result;
}

static int
eval_virtual_height (struct ionogram *ionogram, int layer_id, float freq, float *out)
{
  if (ionogram->layers[layer_id] != NULL)
  {
    if (freq < ionogram->layers[layer_id]->frequencies[0] || 
        freq >= ionogram->layers[layer_id]->frequencies[ionogram->layers[layer_id]->point_count - 1])
      return EVAL_CODE_HOLE;
    
    if (isnan (*out = layer_interpolate_height (ionogram->layers[layer_id], freq)))
      return EVAL_CODE_HOLE;
    else
      return EVAL_CODE_DATA;
  }
  else
    return EVAL_CODE_HOLE;
}

static int
eval_plasma_peak_height (struct ionogram *ionogram, float *out)
{
  int i;
  float max_freq;
  int   max_id;
  
  if (ionogram->plasma_scan_count == 0)
    return EVAL_CODE_NODATA;
 
  max_id = 0;
  max_freq = -INFINITY;
  
  for (i = 0; i < ionogram->plasma_scan_count; i++)
    if (ionogram->plasma_scan_freqs[i] > max_freq)
    {
      max_id = i;
      max_freq = ionogram->plasma_scan_freqs[i];
    }
    
  *out = ionogram->plasma_scan_heights[max_id];
  
  return EVAL_CODE_DATA;
}


static int
eval_muf (struct ionogram *ionogram, float *out)
{
  int i;
  float max_freq;
  int   max_id;
  
  if (ionogram->plasma_scan_count == 0)
    return EVAL_CODE_NODATA;
 
  max_id = 0;
  max_freq = -INFINITY;
  
  for (i = 0; i < ionogram->plasma_scan_count; i++)
    if (ionogram->plasma_scan_freqs[i] > max_freq)
    {
      max_id = i;
      max_freq = ionogram->plasma_scan_freqs[i];
    }
    
  *out = calc_muf (ionogram->plasma_scan_heights[max_id], max_freq);
  
  return EVAL_CODE_DATA;
}

static int
eval_tec (struct ionogram *ionogram, float *out)
{
  int i;
  float sum;
  float delta;
  float density_1, density_2;
  
  if (ionogram->plasma_scan_count == 0)
    return EVAL_CODE_NODATA;
 
  sum = 0;
  delta = 0;
  
  for (i = 1; i < ionogram->plasma_scan_count; i++)
  {
    delta = ionogram->plasma_scan_heights[i] -
            ionogram->plasma_scan_heights[i - 1];
    
    density_1 = pow (ionogram->plasma_scan_freqs[i - 1] / 9.0, 2.0);
    density_2 = pow (ionogram->plasma_scan_freqs[i] / 9.0, 2.0);
    
    sum += 0.5 * delta * (density_1 + density_2);
  }
   
  *out = sum;
  
  return EVAL_CODE_DATA;
}

/* TODO: pass somo extra params would be good */
int 
scaled_data_station_evaluator (struct datasource *source,
                               struct datasource_magnitude *magnitude,
                               struct station_info *station,
                               float sunspot, float freq, float height, 
                               time_t date,
                               float *output, void *data)
{
  struct scaled_data_cache *cache;
  struct ionogram *ionogram;
  struct tm tm;
  char *name;
  char *text;
  int result;
  float value;
  int layer_id;
  
  cache = (struct scaled_data_cache *) data;
  
  cache_lock (cache);
  
  if ((ionogram = unsafe_cache_lookup_ionogram (cache, station, date, 0)) ==
    NULL)
  {
    result = EVAL_CODE_WAIT;
    
    gmtime_r (&date, &tm);
    
    /* This is redundant */
    name = strbuild ("%5s_%04u%03u%02u%02u%02u.SAO", 
      station->name, 
      tm.tm_year + 1900,
      tm.tm_yday + 1, 
      tm.tm_hour, tm.tm_min, tm.tm_sec);
      
    ionowatch_queue_ionogram (name);
  }
  else
  {
    if (ionogram->state_code == 0)
      result = EVAL_CODE_NODATA;
    else
    {
      if (magnitude == virtual_f2_height)
        result = eval_virtual_height (ionogram, IONOGRAM_LAYER_F2, freq, output);
      else if (magnitude == virtual_f1_height)
        result = eval_virtual_height (ionogram, IONOGRAM_LAYER_F1, freq, output);
      else if (magnitude == virtual_e_height)
        result = eval_virtual_height (ionogram, IONOGRAM_LAYER_E, freq, output);
      else if (magnitude == virtual_es_height)
        result = eval_virtual_height (ionogram, IONOGRAM_LAYER_ES, freq, output);
      else if (magnitude == virtual_ea_height)
        result = eval_virtual_height (ionogram, IONOGRAM_LAYER_EA, freq, output);
      else if (magnitude == plasma_frequency)
      {
        value = ionogram_interpolate_plasma_freq (ionogram, height);
        
        if (isnan (value))
          result = EVAL_CODE_NODATA;
        else
        {
          *output = value;
          result = EVAL_CODE_DATA;
        }
      }
      else if (magnitude == plasma_peak_height)
        result = eval_plasma_peak_height (ionogram, output);
      else if (magnitude == muf)
        result = eval_muf (ionogram, output);
      else if (magnitude == tec)
        result = eval_tec (ionogram, output);
      else
        result = EVAL_CODE_NODATA;
    }
  }
  
  cache_unlock (cache);
  
  return result;
}

struct scaled_data_cache *
scaled_data_cache_new (void)
{
  struct scaled_data_cache *new;
  
  new = xmalloc (sizeof (struct scaled_data_cache));
  
  memset (new, 0, sizeof (struct scaled_data_cache));
  
  pthread_mutex_init (&new->mutex, NULL);
  
  return new;
}


void
unsafe_scaled_data_cache_clear_ionograms (struct scaled_data_cache *cache)
{
  int i;
  
  for (i = 0; i < cache->cached_ionogram_count; i++)
    if (cache->cached_ionogram_list[i] != NULL)
      ionogram_destroy (cache->cached_ionogram_list[i]);
      
  if (cache->cached_ionogram_count > 0)
  {
    free (cache->cached_ionogram_list);
    cache->cached_ionogram_count = 0;
  }
}

void
scaled_data_cache_add_ionogram (struct scaled_data_cache *cache,
                                struct ionogram *new)
{
  cache_lock (cache);
  
  PTR_LIST_APPEND (cache->cached_ionogram, new);
  
  cache_unlock (cache);
}

void *
fetch_ionogram_thread (void *data)
{
  struct fetcher_thread_data *thread_data;
  struct ionogram *new;
  struct ionogram_filename fn;
  FILE *fp;
  
  thread_data = (struct fetcher_thread_data *) data;
  
  /* Queue some errors here */
  
  ionowatch_set_status ("Background: fetching %s", thread_data->path);
  
  if (ionogram_parse_filename (thread_data->path, &fn) != -1)
  {
    new = ionogram_new ();
    new->time = fn.time;
    new->station = station_lookup (fn.station);
    
    fn.type = sao_ft->type;
    
    if ((fp = cache_get_ionogram (&fn)) != NULL)
    {
      (sao_ft->parse_callback) (new, fp);
      new->state_code = 1;
      fclose (fp);
    }
    else
      ERROR ("ionogram %s couldn't be retrieved: %s",
        thread_data->path,
        strerror (errno));
  
  
    scaled_data_cache_add_ionogram (datasource_get_userdata (scaled_datasource),
                                  new);
    
    /* TODO: mutex this ?*/
    msg_handler_send (MESSAGE_DATASOURCE_EVENT, DATASOURCE_EVENT_NEW_DATA);
    
  }
  else
    ERROR ("ionogram %s has wrong name format", thread_data->path);
    
  ionowatch_set_status ("Fetch ionogram done");
  
  pthread_mutex_lock (&fetcher_thread_mutex);
  
  PTR_LIST_REMOVE (fetcher_thread, thread_data);
  
  free (thread_data->path);
  free (thread_data);
  
  pthread_mutex_unlock (&fetcher_thread_mutex);
  
  return NULL;
}

static pthread_t thread;

void
ionowatch_queue_ionogram (const char *path)
{
  struct fetcher_thread_data *this;
  int i;
  
  pthread_mutex_lock (&fetcher_thread_mutex);
  
  for (i = 0; i < fetcher_thread_count; i++)
    if (fetcher_thread_list[i] != NULL)
      if (strcmp (fetcher_thread_list[i]->path, path) == 0)
      {
        pthread_mutex_unlock (&fetcher_thread_mutex);
        return;
      }
    
    
  this = xmalloc (sizeof (struct fetcher_thread_data));
  
  this->path = xstrdup (path);  
  pthread_create (&this->thread, NULL, fetch_ionogram_thread, this);
  
  PTR_LIST_APPEND (fetcher_thread, this);
  
  pthread_mutex_unlock (&fetcher_thread_mutex);
}

void
scaled_data_datasource_init (void)
{
  sao_ft = ionogram_filetype_lookup ("SAO");
  scaled_datasource = 
    datasource_register ("scaled-data", "Scaled data source",
                         scaled_data_station_evaluator, 
                         scaled_data_cache_new (), NULL);
                         
  virtual_f2_height = datasource_magnitude_request (
                   "vhF2", "Virtual F2 layer height",
                   MAGNITUDE_TYPE_HEIGHT);
                   
  virtual_f1_height = datasource_magnitude_request (
                   "vhF1", "Virtual F1 layer heigh",
                   MAGNITUDE_TYPE_HEIGHT);
                   
  virtual_e_height = datasource_magnitude_request (
                   "vhE", "Virtual E layer height",
                   MAGNITUDE_TYPE_HEIGHT);
     
  virtual_es_height = datasource_magnitude_request (
                   "vhEs", "Sporadic-E layer virtual height",
                   MAGNITUDE_TYPE_HEIGHT);
                   
  virtual_ea_height = datasource_magnitude_request (
                   "vhEa", "Auroral E layer virtual height",
                   MAGNITUDE_TYPE_HEIGHT);              
        
  plasma_frequency = datasource_magnitude_request (
                   "pf", "Plasma frequency",
                   MAGNITUDE_TYPE_FREQUENCY);
  
  plasma_peak_height = datasource_magnitude_request (
                   "pph", "Plasma peak height",
                   MAGNITUDE_TYPE_REFLECTION);              

  muf = datasource_magnitude_request (
                   "muf", "Maximum usable frequency (MUF)",
                   MAGNITUDE_TYPE_FREQUENCY);              
  
  tec = datasource_magnitude_request (
                   "tec", "Total Electron Content (TEC)",
                   MAGNITUDE_TYPE_TEC);              
  
  datasource_register_magnitude (scaled_datasource, virtual_f2_height);
  datasource_register_magnitude (scaled_datasource, virtual_f1_height);
  datasource_register_magnitude (scaled_datasource, virtual_e_height);
  datasource_register_magnitude (scaled_datasource, virtual_es_height);
  datasource_register_magnitude (scaled_datasource, virtual_ea_height);
  datasource_register_magnitude (scaled_datasource, plasma_frequency);
  datasource_register_magnitude (scaled_datasource, plasma_peak_height);
  datasource_register_magnitude (scaled_datasource, muf);
  datasource_register_magnitude (scaled_datasource, tec);
  
  
}

