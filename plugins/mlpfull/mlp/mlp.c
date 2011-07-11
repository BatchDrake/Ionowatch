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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h>

#include "util.h"
#include "mlp.h"

#define SIGMOID(x)  (1./(1.+exp(-(2.0 * x))))
#define DSIGMOID(x) (2.0 * SIGMOID(x)*(1.-SIGMOID(x)))

numeric_t
sigmoid (numeric_t n)
{
  return SIGMOID (n);
}

numeric_t
dsigmoid (numeric_t n)
{
  return  DSIGMOID (n); 
}

numeric_t
tanhyp (numeric_t n)
{
  return tanh(n);
}

numeric_t
dtanhyp (numeric_t n)
{
  return 1.0 - pow (tanh (n), 2.0);
}

numeric_t
constant (numeric_t ignored)
{
  return 1;
}

numeric_t
dconstant (numeric_t ignored)
{
  return 0;
}

numeric_t
linear (numeric_t n)
{
  return n ;
}

numeric_t
dlinear (numeric_t n)
{
  return 1;
}

struct mlp_neuron *
mlp_neuron_new (int weights, numeric_t (*f) (numeric_t), numeric_t (*df) (numeric_t))
{
  int i;
  struct mlp_neuron *new;
  
  new = xmalloc (sizeof (struct mlp_neuron));
  
  new->weight_count = weights;
  
  if (weights)
  {
    new->weights = xmalloc (weights * sizeof (numeric_t));
      
    for (i = 0; i < weights; i++)
      new->weights[i] = ((numeric_t) rand () / (numeric_t) RAND_MAX - 0.5) / 2.0;
  }
  
  new->f = f;
  new->df = df;
  
  return new;
}

struct mlp *
mlp_new (int input_count, int layer_count, ...)
{
  struct mlp *new;
  va_list ap;
  int i, j;
  int is_bias, is_last_layer, is_last_neuron;
  
  int backwards_neuron_count;
  
  new = xmalloc (sizeof (struct mlp));
  
  new->input_count = input_count;
  new->layer_count = layer_count;
  new->layer_configs = xmalloc (layer_count * sizeof (int));
  new->input_vector = xmalloc ((input_count + 1) * sizeof (numeric_t));
  new->neurons = xmalloc (layer_count * sizeof (struct mlp_neuron **));
  new->pattern_count = 0;
  new->input_vector[new->input_count] = 1.0;
  new->best_mse = INFINITY; /* Don't worry, will improve later */
  
  va_start (ap, layer_count);
  
  for (i = 0; i < layer_count; i++)
  {
    new->layer_configs[i] = va_arg (ap, int);
    
    new->neurons[i] = xmalloc (new->layer_configs[i] * sizeof (struct mlp_neuron *));
    
    for (j = 0; j < new->layer_configs[i]; j++)
    {
      if (i == 0)
        backwards_neuron_count = new->input_count + 1;
      else
        backwards_neuron_count = new->layer_configs[i - 1];
        
      is_last_layer = i == (new->layer_count - 1);
      is_last_neuron = j == (new->layer_configs[i] - 1);
      
      is_bias = !is_last_layer && is_last_neuron;
      
      if (is_bias)
        new->neurons[i][j] = mlp_neuron_new (0.0, constant, dconstant);
      else if (is_last_layer)
        new->neurons[i][j] = mlp_neuron_new (backwards_neuron_count, linear, dlinear); 
      else 
        new->neurons[i][j] = mlp_neuron_new (backwards_neuron_count, sigmoid, dsigmoid);
    }
    
  }
  
  new->output_vector = xmalloc (new->layer_configs[new->layer_count - 1] * sizeof (numeric_t));
  
  
  return new;
}

numeric_t *
mlp_eval (struct mlp *mlp, numeric_t *input)
{
  int i, j, k;
  int is_first_layer, is_last_layer, is_bias;
  int backwards_neuron_count;
  
  numeric_t dot;
  
  memcpy (mlp->input_vector, input, sizeof (numeric_t) * mlp->input_count);
  
  for (i = 0; i < mlp->layer_count; i++)
  {
    is_first_layer = i == 0;
    is_last_layer = i == (mlp->layer_count - 1);
    
    if (is_first_layer)
      backwards_neuron_count = mlp->input_count + 1;
    else
      backwards_neuron_count = mlp->layer_configs[i - 1];
        
    for (j = 0; j < mlp->layer_configs[i]; j++)
    {
      is_bias = mlp->neurons[i][j]->weight_count == 0;
      
      dot = 0.0;
      
      if (!is_bias)
      {
        /* Compute activation for neurons[i][j] */
        for (k = 0; k < backwards_neuron_count; k++)
        { 
          if (is_first_layer)
            dot += mlp->neurons[i][j]->weights[k] * mlp->input_vector[k];
          else
            dot += mlp->neurons[i][j]->weights[k] * mlp->neurons[i - 1][k]->output;
        }
      }
      
      mlp->neurons[i][j]->activation = dot;
      mlp->neurons[i][j]->output = mlp_transfer (mlp->neurons[i][j]);
      
      if (is_last_layer)
        mlp->output_vector[j] = mlp->neurons[i][j]->output;
        
    }
  }
  
  return mlp->output_vector;
}

void
mlp_propagate_error (struct mlp *mlp, numeric_t *target)
{
  int i, j, k;
  int is_last_layer, is_bias;
  int forwards_neuron_count;
  
  numeric_t dot;
  
  for (i = mlp->layer_count - 1; i >= 0; i--)
  {
    is_last_layer = i == mlp->layer_count - 1;
    
    for (j = 0; j < mlp->layer_configs[i]; j++)
    {
      if (is_last_layer)
      {
        if (isnan (mlp->neurons[i][j]->output) || isinf (mlp->neurons[i][j]->output))
          abort ();
          
        if (isnan (target[j]) || isinf (target[j]))
          abort ();
          
        dot = target[j] - mlp->neurons[i][j]->output;
      }
      else
      {
        forwards_neuron_count = mlp->layer_configs[i + 1];
        
        dot = 0.0;
        
        for (k = 0; k < forwards_neuron_count; k++)
        {
          is_bias = mlp->neurons[i + 1][k]->weight_count == 0;
          
          if (!is_bias)
            dot += mlp->neurons[i + 1][k]->weights[j] * mlp->neurons[i + 1][k]->delta;
        }
        
        if (isnan (dot) || isinf (dot))
          abort ();
      }
      
      
      mlp->neurons[i][j]->delta = 
        mlp_transfer_derivative (mlp->neurons[i][j]) * dot;
      
      if (isnan (mlp->neurons[i][j]->delta) || isinf (mlp->neurons[i][j]->delta))
          abort ();
    }
  }
  
  mlp->current_mse = mlp_error (mlp);
  if (mlp->current_mse < mlp->best_mse)
  {
    //NOTICE ("switch %g to %g\n", mlp->best_mse, mlp->current_mse);
    mlp->best_mse = mlp->current_mse;
  }
}

void
mlp_update_weights (struct mlp *mlp, numeric_t eta)
{
  int i, j, k;
  int is_first_layer, is_last_layer, is_bias;
  numeric_t deltaw;

  for (i = 0; i < mlp->layer_count; i++)
  {
    is_first_layer = i == 0;
        
    for (j = 0; j < mlp->layer_configs[i]; j++)
      for (k = 0; k < mlp->neurons[i][j]->weight_count; k++)
      {
        if (is_first_layer)
          deltaw = eta * mlp->neurons[i][j]->delta * mlp->input_vector[k];
        else
          deltaw = eta * mlp->neurons[i][j]->delta * mlp->neurons[i - 1][k]->output;
        
        if (isnan (deltaw) || isinf (deltaw))
          abort ();
          
        mlp->neurons[i][j]->weights[k] += deltaw;
      }
  }
  
  mlp->pattern_count++;     
}

numeric_t 
mlp_error (struct mlp *mlp)
{
  int j;
  
  numeric_t error = 0;
  
  for (j = 0; j < mlp->layer_configs[mlp->layer_count - 1]; j++)
    error += pow (mlp->neurons[mlp->layer_count - 1][j]->delta, 2.0);
  
  return error;
}


