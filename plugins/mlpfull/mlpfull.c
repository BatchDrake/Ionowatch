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
#include <signal.h>
#include <ionowatch.h>
#include <gui.h>
#include <mlp.h>

#include "id3.h"
#include "normal.h"
#include "trainer.h"
#include "evaluators.h"

DECLARE_PLUGIN ("mlp-full");

struct ionowatch_plugin *this_plugin;

struct datasource *full_predictor;
struct datasource *tree_predictor;

struct datasource *h2pf_full_predictor;
struct datasource *h2pf_tree_predictor;

struct datasource *plasma_hack_full_predictor;
struct datasource *plasma_hack_tree_predictor;

struct datasource *plasma_hack_h2pf_full_predictor;
struct datasource *plasma_hack_h2pf_tree_predictor;


struct id3_parser *parser;
struct mlp *mlp;
struct mlp *plasma_profile_mlp;
struct mlp *h2pf_mlp;

static struct datasource_magnitude *virtual_f2_height;
static struct datasource_magnitude *virtual_f1_height;
static struct datasource_magnitude *virtual_e_height;
static struct datasource_magnitude *virtual_es_height;
static struct datasource_magnitude *virtual_ea_height;

static struct datasource_magnitude *real_f2_height;
static struct datasource_magnitude *real_f1_height;
static struct datasource_magnitude *real_e_height;
static struct datasource_magnitude *real_es_height;
static struct datasource_magnitude *real_ea_height;

static struct datasource_magnitude *plasma_frequency;
static struct datasource_magnitude *plasma_peak_height;
static struct datasource_magnitude *muf;
static struct datasource_magnitude *tec;

static int 
station_evaluator (struct datasource *source,
                   struct datasource_magnitude *magnitude,
                   struct station_info *station,
                   float sunspot, float freq, float height,
                   time_t date,
                   float *output, void *data)
{  
  numeric_t *output_vector;
  
  
  int layer_num;
  int virtual_height = 0;
  int tree, h2pf;
  int adjust;
  
  tree = source == tree_predictor || source == h2pf_tree_predictor ||
         source == plasma_hack_tree_predictor || source == plasma_hack_h2pf_tree_predictor;
         
  h2pf = source == h2pf_full_predictor || source == h2pf_tree_predictor ||
         source == plasma_hack_h2pf_full_predictor || source == plasma_hack_h2pf_tree_predictor;
  
  adjust = source == plasma_hack_tree_predictor || source == plasma_hack_h2pf_tree_predictor ||
           source == plasma_hack_full_predictor || source == plasma_hack_h2pf_full_predictor;
     
  if (magnitude == virtual_f2_height)
    return virtual_height_eval (tree,
                                h2pf,
                                adjust,
                                IONOGRAM_LAYER_F2, 
                                station->lat,
                                station->lon,
                                sunspot,
                                freq,  
                                date,
                                output);
                                
  else if (magnitude == virtual_f1_height)
    return virtual_height_eval (tree, 
                                h2pf,
                                adjust,
                                IONOGRAM_LAYER_F1, 
                                station->lat,
                                station->lon,
                                sunspot,
                                freq, 
                                date,
                                output);
                                
  else if (magnitude == virtual_e_height)
    return virtual_height_eval (tree, 
                                h2pf,
                                adjust,
                                IONOGRAM_LAYER_E, 
                                station->lat,
                                station->lon,
                                sunspot,
                                freq, 
                                date,
                                output);
                                
  else if (magnitude == virtual_es_height)
    return virtual_height_eval (tree, 
                                h2pf,
                                adjust,
                                IONOGRAM_LAYER_ES, 
                                station->lat,
                                station->lon,
                                sunspot,
                                freq, 
                                date,
                                output);
                                
  else if (magnitude == virtual_ea_height)
    return virtual_height_eval (tree, 
                                h2pf,
                                adjust,
                                IONOGRAM_LAYER_EA, 
                                station->lat,
                                station->lon,
                                sunspot,
                                freq, 
                                date,
                                output);
                                
  else if (magnitude == real_f2_height)
    return real_height_eval    (tree, 
                                h2pf,
                                IONOGRAM_LAYER_F2, 
                                station->lat,
                                station->lon,
                                sunspot,
                                date,
                                output);
                                
  else if (magnitude == real_f1_height)
    return real_height_eval    (tree, 
                                h2pf,
                                IONOGRAM_LAYER_F1, 
                                station->lat,
                                station->lon,
                                sunspot,
                                date,
                                output);
                                
  else if (magnitude == real_e_height)
    return real_height_eval    (tree, 
                                h2pf,
                                IONOGRAM_LAYER_E, 
                                station->lat,
                                station->lon,
                                sunspot,
                                date,
                                output);
                                
  else if (magnitude == real_es_height)
    return real_height_eval    (tree, 
                                h2pf,
                                IONOGRAM_LAYER_ES, 
                                station->lat,
                                station->lon,
                                sunspot,
                                date,
                                output);
                                
  else if (magnitude == real_ea_height)
    return real_height_eval    (tree, 
                                h2pf,
                                IONOGRAM_LAYER_EA, 
                                station->lat,
                                station->lon,
                                sunspot,
                                date,
                                output);
                                
  else if (magnitude == plasma_frequency)
    return eval_plasma_freq (tree,
                             h2pf,
                             station->lat,
                             station->lon,
                             sunspot, 
                             height,
                             date,
                             output);
                             
  else if (magnitude == plasma_peak_height)
    return eval_plasma_peak_height (tree,
                             h2pf, 
                             station->lat,
                             station->lon,
                             sunspot,
                             date,
                             output);
                             
  else if (magnitude == muf)
    return eval_muf         (tree,
                             h2pf,
                             station->lat,
                             station->lon,
                             sunspot,
                             date,
                             output);
  else if (magnitude == tec)
    return eval_tec         (tree,
                             h2pf,
                             station->lat,
                             station->lon,
                             sunspot,
                             date,
                             output);
  
  else
    return EVAL_CODE_NODATA;  
    
  return EVAL_CODE_DATA;
}

int
id3_init (void)
{
  struct strlist *tagfile;
  char *tree;
  int i;
  FILE *fp;
  
  if ((tree = locate_config_file ("predictor.id3", CONFIG_READ | CONFIG_LOCAL | CONFIG_GLOBAL)) == NULL)
  {
    ERROR ("couldn't open decision tree file, predictions won't work\n");
    return - 1;
  }
  
  if ((fp = fopen (tree, "rb")) == NULL)
  {
    ERROR ("couldn't open %s, predictions won't work\n", tree);
    free (tree);
    return -1;
  }
  
  free (tree);
  
  tagfile = strlist_new ();
  
  strlist_append_string (tagfile, "lat");
  strlist_append_string (tagfile, "lon");
  strlist_append_string (tagfile, "inclination");
  strlist_append_string (tagfile, "sol");
  strlist_append_string (tagfile, "sunspot");
  strlist_append_string (tagfile, "layer");
  strlist_append_string (tagfile, "class");
  
  if ((parser = id3_parser_new_from_tagfile (tagfile, fp)) == NULL)
  {
    ERROR ("couldn't init ID3 tree parser, predictions won't work\n");
    strlist_destroy (tagfile);
    
    return -1;
  }
  
  for (i = 0; i < parser->tree->output_count; i++)
      parser->tree->output_list[i]->number = 
        !strcmp (parser->tree->output_list[i]->name, "ON");
  return 0;
}

int
mlp_init (void)
{
  char *weightfile;
  
  if ((weightfile = locate_config_file ("predictor.mlp", CONFIG_READ | CONFIG_LOCAL | CONFIG_GLOBAL)) == NULL)
  {
    ERROR ("unable to open MLP predictor, predictions won't work\n");
    return -1;
  }
  
  mlp = predictor_build_mlp ();
  
  if (mlp_load_weights (mlp, weightfile) == -1)
  {
    ERROR ("error loading MLP predictor file, predictions won't work\n");
    free (weightfile);
    return -1;
  }
  
  free (weightfile);
  
  return 0;
}

int
plasma_mlp_init (void)
{
  char *weightfile;
  
  
  if ((weightfile = locate_config_file ("plasma.mlp", CONFIG_READ | CONFIG_LOCAL | CONFIG_GLOBAL)) == NULL)
  {
    ERROR ("unable to load plasma profile MLP, plasma predictions won't work\n");
    return -1;
  }
  
  plasma_profile_mlp = plasma_build_mlp ();
  
  if (mlp_load_weights (plasma_profile_mlp, weightfile) == -1)
  {
    ERROR ("error loading plasma profile MLP, plasma predictions won't work\n");
    free (weightfile);
    return -1;
  }
  
  free (weightfile);
  

  
  return 0;
}

static pthread_t trainer_thread;

void
register_plasma_magnitudes (struct datasource *datasource)
{
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

  datasource_register_magnitude (datasource, plasma_frequency);
  datasource_register_magnitude (datasource, plasma_peak_height);
  datasource_register_magnitude (datasource, muf);
  datasource_register_magnitude (datasource, tec);
}

void
register_default_magnitudes (struct datasource *datasource)
{
  virtual_f2_height = datasource_magnitude_request (
                   "vhF2", "F2 layer virtual height",
                   MAGNITUDE_TYPE_HEIGHT);
                   
  virtual_f1_height = datasource_magnitude_request (
                   "vhF1", "F1 layer virtual height",
                   MAGNITUDE_TYPE_HEIGHT);
                   
  virtual_e_height = datasource_magnitude_request (
                   "vhE", "E layer virtual height",
                   MAGNITUDE_TYPE_HEIGHT);

  virtual_es_height = datasource_magnitude_request (
                   "vhEs", "Sporadic-E layer virtual height",
                   MAGNITUDE_TYPE_HEIGHT);
                   
  virtual_ea_height = datasource_magnitude_request (
                   "vhEa", "Auroral E layer virtual height",
                   MAGNITUDE_TYPE_HEIGHT);
  
  real_f2_height = datasource_magnitude_request (
                   "hF2", "F2 layer real height",
                   MAGNITUDE_TYPE_HEIGHT);
                   
  real_f1_height = datasource_magnitude_request (
                   "hF1", "F1 layer real height", 
                   MAGNITUDE_TYPE_HEIGHT);
                   
  real_e_height = datasource_magnitude_request (
                   "hE", "E layer real height", 
                   MAGNITUDE_TYPE_HEIGHT);

  real_es_height = datasource_magnitude_request (
                   "hEs", "Sporadic-E layer real height", 
                   MAGNITUDE_TYPE_HEIGHT);
                   
  real_ea_height = datasource_magnitude_request (
                   "hEa", "Auroral E layer real height", 
                   MAGNITUDE_TYPE_HEIGHT);

                   
  datasource_register_magnitude (datasource, virtual_f2_height);
  datasource_register_magnitude (datasource, virtual_f1_height);
  datasource_register_magnitude (datasource, virtual_e_height);
  datasource_register_magnitude (datasource, virtual_es_height);
  datasource_register_magnitude (datasource, virtual_ea_height);
  datasource_register_magnitude (datasource, real_f2_height);
  datasource_register_magnitude (datasource, real_f1_height);
  datasource_register_magnitude (datasource, real_e_height);
  datasource_register_magnitude (datasource, real_es_height);
  datasource_register_magnitude (datasource, real_ea_height);
}

int 
h2pf_mlp_init (void)
{
  char *weightfile;
  
  if ((weightfile = locate_config_file ("h2pf.mlp", CONFIG_READ | CONFIG_LOCAL | CONFIG_GLOBAL)) == NULL)
  {
    ERROR ("unable to load heigh-to-plasma profile MLP, plasma h2pf predictions won't work\n");
    return -1;
  }
  
  h2pf_mlp = h2pf_build_mlp ();
  
  if (mlp_load_weights (h2pf_mlp, weightfile) == -1)
  {
    ERROR ("error loading h2pf plasma profile MLP, plasma h2pf predictions won't work\n");
    free (weightfile);
    return -1;
  }
  
  free (weightfile);
  
  return 0; 
}

void
plugin_init (void)
{
  sigset_t set;
  
  if (id3_init () == -1)
    return;  

  if (mlp_init () == -1)
    return;
    
  full_predictor = datasource_register ("mlp-full", "MLP raw predictor",
    station_evaluator, NULL, this_plugin);
  tree_predictor = datasource_register ("mlp-tree", "MLP/ID3 predictor",
    station_evaluator, NULL, this_plugin);
  
  register_default_magnitudes (full_predictor);
  register_default_magnitudes (tree_predictor);
  
  if (plasma_mlp_init () != -1)
  {
    register_plasma_magnitudes (full_predictor);
    register_plasma_magnitudes (tree_predictor);
    
    plasma_hack_full_predictor = datasource_register ("plasma-hack-mlp-full", "Adjusted MLP raw predictor",
      station_evaluator, NULL, this_plugin);
      
    plasma_hack_tree_predictor = datasource_register ("plasma-hack-mlp-tree", "Adjusted MLP/ID3 predictor",
      station_evaluator, NULL, this_plugin);
      
    register_default_magnitudes (plasma_hack_full_predictor);
    register_default_magnitudes (plasma_hack_tree_predictor);
    
    register_plasma_magnitudes (plasma_hack_full_predictor);
    register_plasma_magnitudes (plasma_hack_tree_predictor);
  }
  
  if (h2pf_mlp_init () != -1)
  {
    h2pf_full_predictor = datasource_register ("h2pf-mlp-full", "Height-to-frequency MLP raw predictor",
      station_evaluator, NULL, this_plugin);
      
    h2pf_tree_predictor = datasource_register ("h2pf-mlp-tree", "Height-to-frequencyMLP/ID3 predictor",
      station_evaluator, NULL, this_plugin);
      
    register_default_magnitudes (h2pf_full_predictor);
    register_default_magnitudes (h2pf_tree_predictor);
    
    register_plasma_magnitudes (h2pf_full_predictor);
    register_plasma_magnitudes (h2pf_tree_predictor);
    
    plasma_hack_h2pf_full_predictor = datasource_register ("plasma-hack-h2pf-mlp-full", "Adjusted h2pf MLP raw predictor",
      station_evaluator, NULL, this_plugin);
      
    plasma_hack_h2pf_tree_predictor = datasource_register ("plasma-hack-h2pf-mlp-tree", "Adjusted h2pf MLP/ID3 predictor",
      station_evaluator, NULL, this_plugin);
      
    register_default_magnitudes (plasma_hack_h2pf_full_predictor);
    register_default_magnitudes (plasma_hack_h2pf_tree_predictor);
    
    register_plasma_magnitudes (plasma_hack_h2pf_full_predictor);
    register_plasma_magnitudes (plasma_hack_h2pf_tree_predictor);
  }
  

  
  
  
  pthread_create (&trainer_thread, NULL, trainer_thread_entry, NULL);
  
  sigemptyset (&set);
  sigaddset (&set, SIGINT);
  
  pthread_sigmask (SIG_BLOCK, &set, NULL);
}




