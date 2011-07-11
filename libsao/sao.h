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

#ifndef _SAO_H
#define _SAO_H

#include <math.h>
#include <util.h>
#include <index.h>
#include <ionogram.h>

#  ifndef NAN
#  define NAN INFINITY
#  endif /* NAN */

#define SAO_DATA_INDEX_SIZE                    80

#define SAO_GEOPHYSICAL_CONSTANTS              0
#define SAO_SYSTEM_DESCRIPTION                 1
#define SAO_TIME_STAMP_SOUNDER_SETTINGS        2
#define SAO_SCALED_IONOSPHERIC_CHARACTERISTICS 3
#define SAO_ANALYSIS_FLAGS                     4	
#define SAO_DOPPLER_TRANSLATION_TABLE          5

#define SAO_F2_O_VIRTUAL_HEIGHTS               6
#define SAO_F2_O_TRUE_HEIGHTS                  7
#define SAO_F2_O_AMPLITUDES                    8
#define SAO_F2_O_DOPPLER_NUMBERS               9
#define SAO_F2_O_FREQUENCIES                   10

#define SAO_F1_O_VIRTUAL_HEIGHTS               11
#define SAO_F1_O_TRUE_HEIGHTS                  12
#define SAO_F1_O_AMPLITUDES                    13
#define SAO_F1_O_DOPPLER_NUMBERS               14
#define SAO_F1_O_FREQUENCIES                   15

#define SAO_E_O_VIRTUAL_HEIGHTS                16
#define SAO_E_O_TRUE_HEIGHTS                   17
#define SAO_E_O_AMPLITUDES                     18
#define SAO_E_O_DOPPLER_NUMBERS                19
#define SAO_E_O_FREQUENCIES                    20

#define SAO_F2_X_VIRTUAL_HEIGHTS               21
#define SAO_F2_X_AMPLITUDES                    22
#define SAO_F2_X_DOPPLER_NUMBERS               23
#define SAO_F2_X_FREQUENCIES                   24

#define SAO_F1_X_VIRTUAL_HEIGHTS               25
#define SAO_F1_X_AMPLITUDES                    26
#define SAO_F1_X_DOPPLER_NUMBERS               27
#define SAO_F1_X_FREQUENCIES                   28

#define SAO_E_X_VIRTUAL_HEIGHTS                29
#define SAO_E_X_AMPLITUDES                     30
#define SAO_E_X_DOPPLER_NUMBERS                31
#define SAO_E_X_FREQUENCIES                    32

#define SAO_F_ECHO_MEDIAN_AMP                  33
#define SAO_E_ECHO_MEDIAN_AMP                  34
#define SAO_ES_ECHO_MEDIAN_AMP                 35

#define SAO_F2_TRUE_HEIGHTS_COEF_UMLCAR        36
#define SAO_F1_TRUE_HEIGHTS_COEF_UMLCAR        37
#define SAO_E_TRUE_HEIGHTS_COEF_UMLCAR         38

#define SAO_QUAZI_PARABOLIC_SEGMENTS           39
#define SAO_EDIT_FLAGS                         40
#define SAO_VALLEY_DESCRIPTION_UMLCAR          41

#define SAO_ES_O_VIRTUAL_HEIGHTS               42
#define SAO_ES_O_AMPLITUDES                    43
#define SAO_ES_O_DOPPLER_NUMBERS               44
#define SAO_ES_O_FREQUENCIES                   45

#define SAO_EA_O_VIRTUAL_HEIGHTS               46
#define SAO_EA_O_AMPLITUDES                    47
#define SAO_EA_O_DOPPLER_NUMBERS               48
#define SAO_EA_O_FREQUENCIES                   49

#define SAO_TRUE_HEIGHTS                       50
#define SAO_PLASMA_FREQUENCIES                 51
#define SAO_ELECTRON_DENSITIES                 52

#define SAO_URSI_QUALIFYING_LETTERS            53
#define SAO_URSI_DESCRIPTIVE_LETTERS           54
#define SAO_URSI_EDIT_FLAGS                    55

#define SAO_EA_TRUE_HEIGHTS_UMLCAR             56
#define SAO_EA_TRUE_HEIGHTS                    57
#define SAO_EA_PLASMA_FREQUENCIES              58
#define SAO_EA_ELECTRON_DENSITIES              59

typedef struct sao
{
  struct fortran_var *data_index;
  struct fortran_var *groups[SAO_DATA_INDEX_SIZE];
}
sao_t;

static inline int
sao_check_group (sao_t *sao, int group)
{
  if (group < 0 || group >= SAO_DATA_INDEX_SIZE - 1)
  {
    ERROR ("invalid group %d\n", group);
    return -1;
  }
  
  if (sao->groups[group] != NULL)
    return 0;
  else
    return -1;
}

static inline int
sao_group_count (sao_t *sao, int group)
{
  if (group < 0 || group >= SAO_DATA_INDEX_SIZE - 1)
  {
    ERROR ("invalid group %d\n", group);
    return -1;
  }
  
  return FORTRAN_ARRAY_ACCESS (sao->data_index, int, group);
}


static inline int
sao_get_integer (sao_t *sao, int group, int index)
{
  if (sao_check_group (sao, group) == -1)
    return -1;
  
  if (index < 0 || index >= sao->groups[group]->n)
  {
    ERROR ("invalid index %d\n", group);
    return -1;
  }
  
  return FORTRAN_ARRAY_ACCESS (sao->groups[group], int, index);
}

static inline double
sao_get_double (sao_t *sao, int group, int index)
{
  if (sao_check_group (sao, group) == -1)
    return NAN;
  
  if (index < 0 || index >= sao->groups[group]->n)
  {
    ERROR ("invalid index %d\n", group);
    return NAN;
  }
  
  return FORTRAN_ARRAY_ACCESS (sao->groups[group], double, index);
}

static inline char *
sao_get_string (sao_t *sao, int group)
{
  if (sao_check_group (sao, group) == -1)
    return NULL;
  
  return FORTRAN_ARRAY_ACCESS_PTR (sao->groups[group], char, 0);
}


int libsao_init (void);
sao_t *sao_load (FILE *);
void ionogram_import_sao (struct ionogram *, sao_t *);
int parse_sao_file (struct ionogram *, FILE *);
char *sao_build_url (const struct ionogram_filename *, const char *);
void sao_destroy (sao_t *);

#endif /* _SAO_H */


