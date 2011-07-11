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

#ifndef _MLP_H
#define _MLP_H

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

/* Complex numbers MLP? That's cool stuff */

typedef double numeric_t;

struct mlp_neuron
{
  int        weight_count;
  
  numeric_t (*f) (numeric_t);
  numeric_t (*df) (numeric_t);
  
  numeric_t *weights;
  numeric_t  delta;
  numeric_t  output;
  numeric_t  activation;
};

struct mlp
{
  int                  layer_count;
  int                  input_count;
  int                 *layer_configs;
  struct mlp_neuron ***neurons;
  numeric_t            *output_vector;
  numeric_t            *input_vector;
  numeric_t             best_mse;
  numeric_t             current_mse;
  
  int64_t               pattern_count;
};

static inline numeric_t
mlp_transfer (struct mlp_neuron *neuron)
{
  if (isnan (neuron->activation) || isinf (neuron->activation))
    abort ();
    
  return neuron->f (neuron->activation);
}

static inline numeric_t
mlp_transfer_derivative (struct mlp_neuron *neuron)
{
  if (isnan (neuron->activation) || isinf (neuron->activation))
    abort ();
    
  return neuron->df (neuron->activation);
}

numeric_t sigmoid (numeric_t);
numeric_t dsigmoid (numeric_t);
numeric_t tanhyp (numeric_t);
numeric_t dtanhyp (numeric_t);
numeric_t constant (numeric_t);
numeric_t dconstant (numeric_t);
numeric_t linear (numeric_t);
numeric_t dlinear (numeric_t);

struct mlp *mlp_new (int, int, ...);
numeric_t *mlp_eval (struct mlp *, numeric_t *);
void mlp_propagate_error (struct mlp *, numeric_t *);
numeric_t mlp_error (struct mlp *);
void mlp_update_weights (struct mlp *, numeric_t);

#include "mlpfile.h"

#endif /* _MLP_H */

