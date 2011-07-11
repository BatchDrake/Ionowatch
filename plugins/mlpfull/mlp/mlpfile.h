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

#ifndef _MLPFILE_H
#define _MLPFILE_H

#define MLPE_LITTLE 0x00
#define MLPE_BIG    0xff

#ifdef __BIG_ENDIAN__
#  define MLPE_LOCAL MLPE_BIG
#else
#  define MLPE_LOCAL MLPE_LITTLE
#endif

#define MLPFILE_MAGIC "MLP"

#define TRANSFER_TYPE_SIGMOID  0
#define TRANSFER_TYPE_TANH     1
#define TRANSFER_TYPE_LINEAR   2
#define TRANSFER_TYPE_CONSTANT 3 /* For bias nodes, etc */

#define MAX_TRANSFERS          4 

#include <stdint.h>

struct mlpfile_header
{
  uint8_t mh_magic[3];
  uint8_t mh_endian;
  int32_t  mh_layers;
  int32_t  mh_inputs;
  int64_t  mh_pattern_count;
  numeric_t mh_best_mse;
  off_t mh_layer_offsets[1];
};

struct mlpfile_neuron_header
{
  int32_t mn_weight_count;
  int32_t mn_transfer_type;
  numeric_t mn_weights[1];
};

struct mlpfile_layer_header
{
  int32_t   ml_neurons;
  uint32_t ml_neuron_offsets[1];
};

int mlp_save_weights (struct mlp *, const char *);
int mlp_load_weights (struct mlp *, const char *);

#endif /* _MLPFILE_H */

