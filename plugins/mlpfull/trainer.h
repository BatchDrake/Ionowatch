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


#ifndef _TRAINER_H
#define _TRAINER_H

#include <ionowatch.h>
#include <mlp.h>

#include "normal.h"

#ifdef USE_LIBFANN
#  include <doublefann.h>
#endif

#define DEFAULT_EPOCH_COUNT      500000
#define DEFAULT_INFO_INTERVAL    1000
#define DEFAULT_DESIRED_MSE      DESIRED_MSE

struct training_set
{
  PTR_LIST (struct ionogram, training_set);
  PTR_LIST (numeric_t, training_vector);
  PTR_LIST (numeric_t, desired_output);
  
  int epoch_count;
  int training;
  int info_interval;
  int passed_epochs;
  int input_len;
  int output_len;
  int debug_vector;
  
  numeric_t desired_mse;
  numeric_t training_speed;
};

struct strlist *pickup_random_files (void);
struct training_set *training_set_new (void);
void training_set_add_ionogram (struct training_set *, struct ionogram *);
void training_set_add_vectors (struct training_set *, numeric_t *, numeric_t *);
void training_set_set_epoch_count (struct training_set *, int);
void training_set_set_info_interval (struct training_set *, int);
void training_set_set_desired_mse (struct training_set *, numeric_t);
void training_set_set_training_speed (struct training_set *, numeric_t);
void training_set_destroy (struct training_set *);
void training_set_destroy_vectors (struct training_set *);
void *trainer_thread_entry (void *);

#ifdef USE_LIBFANN
struct fann *build_fann (void);
numeric_t training_set_train_on_fann (struct training_set *, struct fann *);
#else
struct mlp *build_mlp (void);
struct mlp *h2pf_build_mlp (void);
struct mlp *plasma_build_mlp (void);
struct mlp *predictor_build_mlp (void);

numeric_t training_set_train_on_mlp (struct training_set *, struct mlp *);
#endif

void training_set_build (struct training_set *);
void h2pf_training_set_build (struct training_set *);
void plasma_training_set_build (struct training_set *);
void predictor_training_set_build (struct training_set *);

#endif /* _TRAINER_H */

