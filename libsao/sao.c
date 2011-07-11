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

#include <util.h>

#include <fortran.h>
#include <sao.h>

#include <index.h>
#include <ionogram.h>

static char *sao_formats_str[SAO_DATA_INDEX_SIZE] = 
{
  "16F7.3",    "A120",      "120A1",     "15F8.3",    "60I2",     "16F7.3",    
  "15F8.3",    "15F8.3",    "40I3",      "120I1",     "15F8.3",   "15F8.3",
  "15F8.3",    "40I3",      "120I1",     "15F8.3",    "15F8.3",   "15F8.3",    
  "40I3",      "120I1",     "15F8.3",    "15F8.3",    "40I3",     "120I1", 
  "15F8.3",    "15F8.3",    "40I3",      "120I1",     "15F8.3",   "15F8.3",    
  "40I3",      "120I1",     "15F8.3",    "40I3",      "40I3",     "40I3",   
  "10E11.6E1", "10E11.6E1", "10E11.6E1", "6E20.12E2", "120I1",    "10E11.6E1", 
  "15F8.3",    "40I3",      "120I1",     "15F8.3",    "15F8.3",   "40I3", 
  "120I1",     "15F8.3",    "15F8.3",    "15F8.3",    "15E8.3E1", "120A1",     
  "120A1",     "120I1",     "10E11.6E1", "15F8.3",    "15F8.3",   "15E8.3E1"

};
/* NOTE: struct fortran_format holds a dynamically-allocated buffer 
   with the sole purpose of being filled by __fortran_read_atom. Since
   sao_formats are going to be modified by further calls to sao_load,
   sao_loads becomes non-reentrant.
   
   Handle with care at the time of using threads. I would be a good idea to 
   protect sao_formats with mutexes before screwing something up. 
*/
   
static struct fortran_format sao_formats[SAO_DATA_INDEX_SIZE];
static struct fortran_format index_format;

static sao_t *
sao_new (void)
{
  sao_t *new;
  
  new = xmalloc (sizeof (sao_t));
  
  memset (new, 0, sizeof (sao_t));
  
  return new;
}

void
sao_destroy (sao_t *sao)
{
  int i;
  
  for (i = 0; i < SAO_DATA_INDEX_SIZE; i++)
    if (sao->groups[i] != NULL)
      fortran_var_destroy (sao->groups[i]);
      
  if (sao->data_index != NULL)
    fortran_var_destroy (sao->data_index);
    
  free (sao);
}

sao_t *
sao_load (FILE *fp)
{
  struct fortran_stream *stream;
  sao_t *new;
  int i, n;
  
  stream = fortran_stream_from_fp (fp);
  
  new = sao_new ();
    
  if ((new->data_index = 
    fortran_scan (stream, &index_format, SAO_DATA_INDEX_SIZE)) == NULL)
  {
    ERROR ("can't read index\n");
    fortran_stream_destroy (stream);
    return NULL;
  }
  
  
  if (new->data_index->n != SAO_DATA_INDEX_SIZE)
  {
    ERROR ("data index size too short at offset %d, len %d\n", ftell (fp), new->data_index->n);
    fortran_stream_destroy (stream);
    sao_destroy (new);    
    return NULL;
  }
  
  /* Last element in index is version number, we can omit that */
  for (i = 0; i < SAO_DATA_INDEX_SIZE - 1; i++)
  {
    if ((n = FORTRAN_ARRAY_ACCESS (new->data_index, int, i)) > 0)
    {
      if ((new->groups[i] = fortran_scan (stream, &sao_formats[i], n)) ==
        NULL)
      {
        ERROR ("invalid group %d\n", i);
        
        sao_destroy (new);    
        fortran_stream_destroy (stream);
        
        return NULL;    
      }
      
      if (new->groups[i]->n != n)
      {
        ERROR ("incomplete group %d (got %d out of %d), fixing\n", i, new->groups[i]->n, n);
        
        FORTRAN_ARRAY_ACCESS (new->data_index, int, i) = new->groups[i]->n;
      }
    }
  }
  
  fortran_stream_destroy (stream);
  
  return new;
}

static int
__add_layer (struct ionogram *ionogram, 
             sao_t *sao, 
             int type,
             int polarization,
             int freq_group,
             int height_group)
{
  int card, i;
  struct layer_info *info;
  
  if (sao_check_group (sao, freq_group) == -1 ||
      sao_check_group (sao, height_group))
    return 0;
    
  if ((card = sao_group_count (sao, freq_group)) !=
      sao_group_count (sao, height_group))
  {
    ERROR ("group cardinality inconsistency found (%d != %d)\n",
      sao_group_count (sao, freq_group), sao_group_count (sao, height_group));
    return 0;
  }
  
  info = layer_info_new (type, polarization, card);
  
  for (i = 0; i < card; i++)
  {
    info->heights[i] = sao_get_double (sao, height_group, i);
    info->frequencies[i] = sao_get_double (sao, freq_group, i);
  }
  
  ionogram_add_layer (ionogram, info);
  
  return 1;
}
 
void
ionogram_import_sao (struct ionogram *ionogram, sao_t *sao)
{
  int i;
  
  if (sao_check_group (sao, SAO_TRUE_HEIGHTS) == 0)
  {
    if (sao_group_count (sao, SAO_TRUE_HEIGHTS) ==
        sao_group_count (sao, SAO_PLASMA_FREQUENCIES))
    {
      ionogram->plasma_scan_count = sao_group_count (sao, SAO_TRUE_HEIGHTS);
      
      ionogram->plasma_scan_heights = xmalloc (sizeof (float) * ionogram->plasma_scan_count);
      ionogram->plasma_scan_freqs   = xmalloc (sizeof (float) * ionogram->plasma_scan_count);
      
      for (i = 0; i < ionogram->plasma_scan_count; i++)
      {
        ionogram->plasma_scan_heights[i] = sao_get_double (sao, SAO_TRUE_HEIGHTS, i);
        ionogram->plasma_scan_freqs[i] = sao_get_double (sao, SAO_PLASMA_FREQUENCIES, i);
      }
    }
    else
      ERROR ("true height profile inconsistent (%d != %d)\n",
        sao_group_count (sao, SAO_TRUE_HEIGHTS),
        sao_group_count (sao, SAO_PLASMA_FREQUENCIES));
  }
  
  (void) __add_layer (ionogram, sao, 
    IONOGRAM_LAYER_F2, IONOGRAM_POLARIZATION_O,
    SAO_F2_O_FREQUENCIES, SAO_F2_O_VIRTUAL_HEIGHTS);
    
  (void) __add_layer (ionogram, sao, 
    IONOGRAM_LAYER_F1, IONOGRAM_POLARIZATION_O,
    SAO_F1_O_FREQUENCIES, SAO_F1_O_VIRTUAL_HEIGHTS);
    
  (void) __add_layer (ionogram, sao,
    IONOGRAM_LAYER_E, IONOGRAM_POLARIZATION_O,
    SAO_E_O_FREQUENCIES, SAO_E_O_VIRTUAL_HEIGHTS);
    
  (void) __add_layer (ionogram, sao, 
    IONOGRAM_LAYER_ES, IONOGRAM_POLARIZATION_O,
    SAO_ES_O_FREQUENCIES, SAO_ES_O_VIRTUAL_HEIGHTS);
    
  (void) __add_layer (ionogram, sao,
    IONOGRAM_LAYER_EA, IONOGRAM_POLARIZATION_O,
    SAO_EA_O_FREQUENCIES, SAO_EA_O_VIRTUAL_HEIGHTS);
  
  (void) __add_layer (ionogram, sao, 
    IONOGRAM_LAYER_F2, IONOGRAM_POLARIZATION_X,
    SAO_F2_X_FREQUENCIES, SAO_F2_X_VIRTUAL_HEIGHTS);
    
  (void) __add_layer (ionogram, sao, 
    IONOGRAM_LAYER_F1, IONOGRAM_POLARIZATION_X,
    SAO_F1_X_FREQUENCIES, SAO_F1_X_VIRTUAL_HEIGHTS);
    
  (void) __add_layer (ionogram, sao,
    IONOGRAM_LAYER_E, IONOGRAM_POLARIZATION_X,
    SAO_E_X_FREQUENCIES, SAO_E_X_VIRTUAL_HEIGHTS);
}

int
parse_sao_file (struct ionogram *ionogram, FILE *fp)
{
  sao_t *sao;
  
  if ((sao = sao_load (fp)) == NULL)
    return -1;
    
  ionogram_import_sao (ionogram, sao);
  
  sao_destroy (sao);
  
  return 0;
}

char *
sao_build_url (const struct ionogram_filename *fn, const char *baseurl)
{
  return strbuild ("%s/%s/individual/%04d/%03d/scaled/",
    baseurl, fn->station, fn->year, fn->yday);
    
}

int
libsao_init (void)
{
  int i;
  
  if (fortran_parse_format ("I3", &index_format) == -1)
    return -1;
    
  for (i = 0; i < SAO_DATA_INDEX_SIZE; i++)
  {
    if (sao_formats_str[i] == NULL)
      continue;
    
    if (fortran_parse_format (sao_formats_str [i], &sao_formats[i]) == -1)
    {
      ERROR ("error parsing format %s at field %d (bug)\n", 
        sao_formats_str[i], i);
        
      return -1;
    }
  }
  
  return 0;
}

