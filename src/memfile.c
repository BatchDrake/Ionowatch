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

#include "libini.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int mem_file_add_line (file_t *dest, char *line)
{
  char **nptr;
  char *sdup;
  
  sdup = xstrdup (line);
  
  nptr = (char **) xrealloc (dest->fc_lines, (dest->fc_line_count + 1) * sizeof (char *));
  
  dest->fc_lines = nptr;
  dest->fc_lines[dest->fc_line_count] = sdup;
  dest->fc_line_count++;
  
  return 0;
}

int mem_file_insert (file_t *file, int line, char *text)
{
  void *p;
  char *dup;
  int i;
  
  dup = xstrdup (text);
  
  if (line < 0 || line >= file->fc_line_count)
    return -1;
  
  p = xrealloc (file->fc_lines, (file->fc_line_count + 1) * sizeof (char *));

  file->fc_lines = (char **) p;
  file->fc_line_count++;
  
  for (i = file->fc_line_count; i > line; i--)
    file->fc_lines[i] = file->fc_lines[i - 1]; /* Vamos desplazando las líneas, copiando en cada una
                    la dirección de la anterior */
  
  file->fc_lines[line] = dup;
  
  return 0;
}

int mem_file_line_delete (file_t *file, int line)
{
  void *p;
  int i;
  
  if (line < 0 || line >= file->fc_line_count)
    return -1;
  
  /* Este es el caso más simple */
  if (file->fc_line_count == 1)
  {
    free (file->fc_lines[0]);
    free (file->fc_lines);
    file->fc_lines = NULL;
    file->fc_line_count--;
    
    return 0;
  }
  
  /* Para todo lo demás, MasterCard */
  for (i = line; i < file->fc_line_count - 1; i++)
    file->fc_lines[i] = file->fc_lines[i + 1];
    
  p = realloc (file->fc_lines, (file->fc_line_count - 1) * sizeof (char *));
  
  if (p == NULL)
    return -1;
  
  file->fc_line_count--;
  
  return 0;
}

void clear_mem_file (file_t *file)
{
  int i;
  
  if (file->fc_lines)
  {
    for (i = 0; i < file->fc_line_count; i++)
      free (file->fc_lines[i]);
    free (file->fc_lines);
    file->fc_lines = NULL;
  }
  
  file->fc_line_count = 0;
  free (file->fc_name);
}

int init_mem_file_blank (file_t *file, char *name)
{
  memset (file, 0, sizeof (file_t));
  
  file->fc_name = xstrdup (name);
  
  return 0;
}

int flush_mem_file (file_t* file)
{
  FILE *fp;
  int i;
  
  fp = fopen (file->fc_name, "wb");
  
  if (fp == NULL)
    return -1;
  
  for (i = 0; i < file->fc_line_count; i++)
    fprintf (fp, "%s%s", file->fc_lines[i], (file->fc_line_end == LE_LF) ? "\n" : "\r\n");
  
  fclose (fp);
  
  return 0;
}

int load_mem_file (char *path, file_t *dest)
{
  FILE *fp;
  int i;
  char linebuf[1024];
  
  fp = fopen (path, "rb");
  if (fp == NULL)
    return -1;
  
  dest->fc_line_count = 0;
  dest->fc_lines = NULL;
  dest->fc_line_end = LE_LF;
  
  dest->fc_name = xstrdup (path);
  
  while (!feof (fp))
  {
    if (fgets (linebuf, 1024, fp) == NULL)
      break;
    for (i = strlen (linebuf); i >= 0; i--)
    {
      if (linebuf[i] == '\r')
      {
        dest->fc_line_end = LE_CRLF;
        linebuf[i] = '\0';
      }
      else if (linebuf[i] == '\n')
        linebuf[i] = '\0';
    }
    
    if (mem_file_add_line (dest, linebuf) == -1)
    {
      clear_mem_file (dest);
      fclose (fp);
      return -1;
    }
  }
  
  fclose (fp);
  return 0;
}
