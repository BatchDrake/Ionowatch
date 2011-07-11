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
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <util.h>

#include "mlp.h"
#include "mlpfile.h"

static numeric_t (*transfers[MAX_TRANSFERS]) (numeric_t) =
  {sigmoid, tanhyp, linear, constant};
  
/* TODO: We should do the same we do with malloc - but with fwrite */

static inline int
write_off_at (FILE *fp, off_t offset, off_t what)
{
  off_t saved;
  int result;
  
  saved = ftell (fp);
  fseek (fp, offset, SEEK_SET);
  
  result = fwrite (&what, sizeof (off_t), 1, fp);
  
  fseek (fp, saved, SEEK_SET);
  
  return result;
}

int
mlp_save_weights (struct mlp *mlp, const char *path)
{
  int i, j, n;
  off_t off = 0;
  off_t off_list;
  
  FILE *fp;
  
  struct mlpfile_header header;
  struct mlpfile_layer_header layer;
  struct mlpfile_neuron_header neuron;
  
  if ((fp = fopen (path, "wb")) == NULL)
  {
    ERROR ("couldn't open %s for writing: %s\n", path, strerror (errno));
    return -1;
  }
  
  memcpy (header.mh_magic, MLPFILE_MAGIC, 3);
  
  header.mh_endian = MLPE_LOCAL;
  header.mh_layers = mlp->layer_count;
  header.mh_inputs = mlp->input_count;
  header.mh_layer_offsets[0] = 0;
  header.mh_pattern_count = mlp->pattern_count;
  header.mh_best_mse = mlp->best_mse;
  
  if (fwrite (&header, sizeof (struct mlpfile_header), 1, fp) < 1)
  {
    ERROR ("frwrite write error: %s\n", strerror (errno));
    fclose (fp);
    return -1;
  }
  
  /* Placeholders for offset (note that header has the first offset already) */
  
  for (n = 1; n < header.mh_layers; n++)
    (void) fwrite (&off, sizeof (off_t), 1, fp);
    
  for (i = 0; i < header.mh_layers; i++)
  {
    layer.ml_neurons = mlp->layer_configs[i];
    layer.ml_neuron_offsets[0] = 0;
    
    (void) write_off_at (fp, sizeof (struct mlpfile_header) + (i - 1) * sizeof (off_t), ftell (fp));
    if (fwrite (&layer, sizeof (struct mlpfile_layer_header), 1, fp) < 1)
    {
      ERROR ("frwrite write error: %s\n", strerror (errno));
      fclose (fp);
      return -1;
    }
    
    off_list = ftell (fp) - sizeof (off_t);
    
    for (n = 1; n < mlp->layer_configs[i]; n++)
      (void) fwrite (&off, sizeof (off_t), 1, fp);
      
    for (j = 0; j < mlp->layer_configs[i]; j++)
    {
      for (n = 0; n < MAX_TRANSFERS; n++)
        if (mlp->neurons[i][j]->f == transfers[n])
          break;
          
          
      if (n == MAX_TRANSFERS)
      {
        ERROR ("unregistered transfer function %p in neuron %d, %d\n", mlp->neurons[i][j]->f, i, j);
        fclose (fp);
        return -1;
      }
      
      neuron.mn_transfer_type = n;
      neuron.mn_weight_count = mlp->neurons[i][j]->weight_count;
      
      
      (void) write_off_at (fp, off_list + j * sizeof (off_t), ftell (fp));
      
      if (fwrite (&neuron, sizeof (struct mlpfile_neuron_header), 1, fp) < 1)
      {
        ERROR ("frwrite write error: %s\n", strerror (errno));
        fclose (fp);
        return -1;
      }
      
      fseek (fp, -sizeof (numeric_t), SEEK_CUR);
      for (n = 0; n < mlp->neurons[i][j]->weight_count; n++)
        if (fwrite (&mlp->neurons[i][j]->weights[n], sizeof (numeric_t), 1, fp) < 1)
        {
          ERROR ("frwrite write error: %s\n", strerror (errno));
          fclose (fp);
          return -1;
        }
    }
  }
  
  fclose (fp);
  
  return 0;
}

/* TODO: HAZARDOUS: fill with checks until exhaustion */
int
mlp_load_weights (struct mlp *mlp, const char *path)
{
  int fd;
  void *file;
  int i, j, k;
  size_t len;
  
  struct mlpfile_header *header;
  struct mlpfile_layer_header *layer;
  struct mlpfile_neuron_header *neuron;
  
  if ((fd = open (path, O_RDONLY)) == -1)
  {
    ERROR ("couldn't open %s for reading: %s\n", path, strerror (errno));
    return -1;
  }
  
  len = lseek (fd, 0, SEEK_END);
  
  file = mmap (NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  
  close (fd);
  
  if (file == (void *) -1)
  {
    ERROR ("coudln't mmap %s: %d\n", path, strerror (errno));
    return -1;
  }
   
  header = (struct mlpfile_header *) file;
  
  if (memcmp (header->mh_magic, MLPFILE_MAGIC, 3) != 0)
  {
    ERROR ("%s: not a saved MLP\n", path);
    munmap (file, len);
    return -1;
  }
  
  if (header->mh_endian != MLPE_LOCAL)
  {
    ERROR ("%s: MLP weights are of foreign endianness, can't continue\n", path);
    munmap (file, len);
    return -1;
  }
  
  mlp->pattern_count = header->mh_pattern_count;
  mlp->best_mse = header->mh_best_mse;
  
  if (mlp->input_count != header->mh_inputs)
  {
    ERROR ("%s: MLP input count mismatch (%d != %d)\n", 
      path, mlp->input_count, header->mh_inputs);
      
    munmap (file, len);
    return -1;
  }
  
  if (mlp->layer_count != header->mh_layers)
  {
    ERROR ("%s: MLP layer count mismatch (%d != %d)\n", 
      path, mlp->layer_count, header->mh_layers);
    munmap (file, len);
    return -1;
  }
  
  for (i = 0; i < mlp->layer_count; i++)
  {
    layer = (struct mlpfile_layer_header *) (file + header->mh_layer_offsets[i]);
    
    if (layer->ml_neurons != mlp->layer_configs[i])
    {
      ERROR ("%s: layer %d neuron count mismatch (%d != %d)\n", 
        path, i, mlp->layer_configs[i], layer->ml_neurons);
      munmap (file, len);
      return -1;
    }
    
    for (j = 0; j < layer->ml_neurons; j++)
    {
      neuron = (struct mlpfile_neuron_header *) (file + layer->ml_neuron_offsets[j]);
      
      if (neuron->mn_weight_count != mlp->neurons[i][j]->weight_count)
      {
        ERROR ("%s: neuron %d,%d backwards weight count mismatch (%d != %d)\n", 
          path, i, j, neuron->mn_weight_count, mlp->neurons[i][j]->weight_count);
        munmap (file, len);
        return -1;
      }
    
      for (k = 0; k < neuron->mn_weight_count; k++)
        mlp->neurons[i][j]->weights[k] = neuron->mn_weights[k];
    }
  }

  
  munmap (file, len);
  return 0;
}

