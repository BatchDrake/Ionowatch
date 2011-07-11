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
#include <ctype.h>

#include "util.h"
#include "fortran.h"

struct fortran_stream *
fortran_stream_new (void *data, 
                    size_t (*read_callback) (void *, void *, size_t),
                    int    (*seek_callback) (void *, off_t, int))
{
  struct fortran_stream *new;
  
  new = xmalloc (sizeof (struct fortran_stream));
  
  new->data = data;
  new->read = read_callback;
  new->seek = seek_callback;
}

size_t
fortran_read (struct fortran_stream *stream, void *buf, size_t size)
{
  return stream->read (stream->data, buf, size);
}

int
fortran_seek (struct fortran_stream *stream, off_t offset, int whence)
{
  return stream->seek (stream->data, offset, whence);
}

void
fortran_stream_destroy (struct fortran_stream *stream)
{  
  free (stream);
}

static size_t
f_fread (void *fp, void *buf, size_t size)
{
  int i;
  size_t ret;
  int errno_before;
  
  errno_before = errno;
  for (i = 0; i < size; i++)
    if ((ret = fread (buf + i, 1, 1, (FILE *) fp)) == 0) 
      break;
    else if (*((char *) buf + i) =='\r' || *((char *) buf + i) == '\n')
    {
      i++;
      break;
    }
    else if (ret == -1)
      return -1;
      
  ((char *) buf) [i] = '\0';
 
  return i;
}

static int
f_fseek (void *fp, off_t offset, int whence)
{
  return fseek ((FILE *) fp, offset, whence);
}

struct fortran_stream *
fortran_stream_fopen (const char *file)
{
  struct fortran_stream *stream;
  FILE *fp;
  
  if ((fp = fopen (file, "rb")) == NULL)
    return NULL;
    
  stream = fortran_stream_new ((void *) fp, f_fread, f_fseek);
  
  return stream;
}


struct fortran_stream *
fortran_stream_from_fp (FILE *fp)
{
  struct fortran_stream *stream;
  
  stream = fortran_stream_new ((void *) fp, f_fread, f_fseek);
  
  return stream;
}

void
fortran_stream_fclose (struct fortran_stream *stream)
{
  fclose ((FILE *) stream->data);
  fortran_stream_destroy (stream);
}

int
fortran_getchar (struct fortran_stream *stream)
{
  unsigned char b;
  
  if (fortran_read (stream, &b, 1) < 1)
    return EOF;
  else
    return b;
}

int
fortran_rew (struct fortran_stream *stream)
{
  return fortran_seek (stream, -1, FORTRAN_SEEK_CUR);
}


int
fortran_parse_format (char *format, struct fortran_format *dest)
{
  char *fptr;
  char *tmp;
  
  
  dest->exp_subtype = 0;
  dest->positioning = (int) strtoul (format, &fptr, 10);
  
  if (fptr == format) /* No integer before format */
    dest->positioning = 0;
  
  if (!*fptr)
  {
    ERROR ("FORTRAN format specifier expected at column %d\n", 
      (int) ((unsigned int) fptr - (unsigned int) format));
    return -1;
  }
  
  switch (dest->type = toupper (*fptr++))
  {
    case FORTRAN_INTEGER:
      /* TODO: use #defines */
      dest->width = (int) strtoul (fptr, &tmp, 10);
      if (tmp == fptr)
      {
        ERROR ("FORTRAN width specifier not found at column %d\n",
          (int) ((unsigned int) fptr - (unsigned int) format));
          
        return -1;
      }
      
      fptr = tmp;
      
      /* TODO: do the same */
      if (*fptr)
      {
        if (*fptr != '.') /* This is not always true! */
        {
          ERROR ("expected period at column %d\n",
            (int) ((unsigned int) fptr - (unsigned int) format));
            
          return -1;
        }
        
        fptr++;
      
      
        dest->minimum = (int) strtoul (fptr, &tmp, 10);
        if (tmp == fptr)
        {
          ERROR ("FORTRAN minimum width specifier not found at column %d\n",
            (int) ((unsigned int) fptr - (unsigned int) format));
            
          return -1;
        }
        
        fptr = tmp;
      }
      else
        dest->minimum = 1;
      
      break;
      
    case FORTRAN_REAL_DECIMAL:
    case FORTRAN_REAL_EXPONENTIAL:
    
      if (dest->type == FORTRAN_REAL_EXPONENTIAL)
      {
        if (!isdigit (*fptr))
        {
          dest->exp_subtype = *fptr++;
          
          if (dest->exp_subtype != FORTRAN_SCIENTIFIC_SUFFIX &&
              dest->exp_subtype != FORTRAN_ENGINEERING_SUFFIX)
          {
            ERROR ("FORTRAN exponent subtype specifier unknown at "
                   "column %d (expected 'S' or 'N')\n", 
              (int) ((unsigned int) fptr - (unsigned int) format));
              
            return -1;
          } 
        }
      }
      /* TODO: use #defines */
      dest->width = (int) strtoul (fptr, &tmp, 10);
      if (tmp == fptr)
      {
        ERROR ("FORTRAN width specifier not found at column %d\n",
          (int) ((unsigned int) fptr - (unsigned int) format));
          
        return -1;
      }
      
      fptr = tmp;
      
      if (*fptr != '.')
      {
        ERROR ("expected period at column %d\n",
          (int) ((unsigned int) fptr - (unsigned int) format));
          
        return -1;
      }
      
      fptr++;
    
    
      dest->decimals = (int) strtoul (fptr, &tmp, 10);
      if (tmp == fptr)
      {
        ERROR ("FORTRAN decimal specifier not found at column %d\n",
          (int) ((unsigned int) fptr - (unsigned int) format));
          
        return -1;
      }
      
      fptr = tmp;

      /* TODO: do the same */
      if (*fptr)
      {
        if (*fptr != 'E') /* This is not always true! */
        {
          ERROR ("expected 'E' at column %d\n",
            (int) ((unsigned int) fptr - (unsigned int) format));
            
          return -1;
        }
        
        fptr++;
      
      
        dest->exponent = (int) strtoul (fptr, &tmp, 10);
        if (tmp == fptr)
        {
          ERROR ("FORTRAN exponent width specifier not found at column %d\n",
            (int) ((unsigned int) fptr - (unsigned int) format));
            
          return -1;
        }
        
        fptr = tmp;
      }
      
      break;
    
    case FORTRAN_LOGICAL:
      /* TODO: use #defines */
      dest->width = (int) strtoul (fptr, &tmp, 10);
      if (tmp == fptr)
      {
        ERROR ("FORTRAN width specifier not found at column %d\n",
          (int) ((unsigned int) fptr - (unsigned int) format));
          
        return -1;
      }
      
      fptr = tmp;
      
      break;
    
    case FORTRAN_CHARACTER:
      /* TODO: use #defines */
      dest->width = (int) strtoul (fptr, &tmp, 10);
      if (tmp == fptr)
        dest->width = 1;
      
      fptr = tmp;
      
      break;
    
    default:
      ERROR ("FORTRAN format specifier not recognized '%c'\n", dest->type);
      return -1;
  }
  
  dest->reading_buf = xmalloc (dest->width + 1);
  dest->reading_buf[dest->width] = '\0';
  
  return 0;
}

void
fortran_format_destroy (struct fortran_format *format)
{
  free (format->reading_buf);
}

static int
fortran_get_format_size (struct fortran_format *format)
{
  switch (format->type)
  {
    case FORTRAN_CHARACTER:
    case FORTRAN_LOGICAL:
      return format->width;
      
    case FORTRAN_REAL_DECIMAL:
    case FORTRAN_REAL_EXPONENTIAL:
      return sizeof (double);
      
    case FORTRAN_INTEGER:
      return sizeof (int);
  }
  
  return -1;
}

/* TODO: implement this more wisely */
static int
__fortran_read_atom (struct fortran_stream *stream,
                     struct fortran_format *format,
                     void *buf)
{
  int n;
  double q;
  char *ptr, *first;
  int width;
  
  n = 0;
  
  if ((width = fortran_read (stream, format->reading_buf, format->width)) < 1)
    return 0;
        
  switch (format->type)
  {
    case FORTRAN_CHARACTER:
      memcpy (buf, format->reading_buf, width);
      break;
    
    case FORTRAN_INTEGER:
      if (width != format->width)
        return 0;
        
        
      first = format->reading_buf;
      
      while (isspace (*first))
        first++;
        
      if (!sscanf (format->reading_buf, "%i", &n))
      {
        ERROR ("can't get an integer from \"%s\" (width is %d)\n", format->reading_buf, format->width);
        return 0;
      }
      
      memcpy (buf, &n, sizeof (int));
      break;
      
    case FORTRAN_REAL_DECIMAL:
    case FORTRAN_REAL_EXPONENTIAL:
      if (width != format->width)
        return 0;
        
      
      first = format->reading_buf;
      
      while (isspace (*first))
        first++;
        
      q = strtod (first, &ptr);
      
      if (first == ptr)
      {
        ERROR ("can't get a real from \"%s\", first is %c\n", format->reading_buf, *first);
        return 0;
      }
      
      memcpy (buf, &q, sizeof (double));
      break;
      
    default:
      ERROR ("FORTRAN format data corruption (bug, type: %c)\n", format->type);
      return 0;
  }
  
  return 1;
}

struct fortran_var *
fortran_scan (struct fortran_stream *stream,
              struct fortran_format *format,
              int times)
{
  
  struct fortran_var *var;
  int c;
  int i;
  int element_size;
  
  if ((element_size = fortran_get_format_size (format)) == -1)
  {
    ERROR ("FORTRAN format data corruption (bug, unk type: %d)\n",
      format->type);
    return NULL;
  }
  
  var = xmalloc (sizeof (struct fortran_var));
 
  var->type = format->type;
  var->n    = times;
  var->buf  = xmalloc (times * element_size);
  
  for (i = 0; i < times;)
  {
    while ((c = fortran_getchar (stream)) != EOF)
      if (c != '\r' && c != '\n')
      {
        fortran_rew (stream);
        break;
      }

    if (c == EOF)
    {
      NOTICE ("EOF While reading %d/%d %c with width %d\n", i + 1, times, format->type, format->width);
      break;
    }
    
    if (!__fortran_read_atom (stream, format, var->buf + i * element_size))
    {
      ERROR ("While reading %d/%d %c with width %d\n", i + 1, times, format->type, format->width);
    }
    else
      i++;
  }
  
  /* Let's go to the end of the line */
  fortran_rew (stream);
  while ((c = fortran_getchar (stream)) != EOF)
    if (c == '\r' || c == '\n')
      break;

  var->n = i;
  
  return var;
}

void
fortran_var_destroy (struct fortran_var *var)
{
  free (var->buf);
  
  free (var);
}


