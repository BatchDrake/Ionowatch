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

#ifndef _FORTRAN_H
#define _FORTRAN_H

#include <sys/types.h>
#include <unistd.h>

typedef char   FORTRAN_CHARACTER_T;
typedef int    FORTRAN_INTEGER_T;
typedef double FORTRAN_REAL_T;
typedef char   FORTRAN_LOGICAL_T;

/* http://www.cs.mtu.edu/~shene/COURSES/cs201/NOTES/chap05/format.html */

#define FORTRAN_CHARACTER          'A'
#define FORTRAN_REAL_DECIMAL       'F'
#define FORTRAN_REAL_EXPONENTIAL   'E'
#define FORTRAN_LOGICAL            'L'
#define FORTRAN_INTEGER            'I'

#define FORTRAN_SCIENTIFIC_SUFFIX  'S'
#define FORTRAN_ENGINEERING_SUFFIX 'N'

#define FORTRAN_SEEK_SET SEEK_SET
#define FORTRAN_SEEK_CUR SEEK_CUR
#define FORTRAN_SEEK_END SEEK_END

#define FORTRAN_ARRAY_ACCESS(var, c_type, index)    \
  (*((c_type *) ((var)->buf + (index) * sizeof (c_type))))

#define FORTRAN_ARRAY_ACCESS_PTR(var, c_type, index)    \
  ((c_type *) ((var)->buf + (index) * sizeof (c_type)))


struct fortran_format
{
  char type;
  char exp_subtype;
  
  int positioning;
  int width;
  int minimum;
  int decimals;
  int exponent;
  
  char *reading_buf;
};

struct fortran_stream
{
  void *data;
  size_t (*read) (void *, void *, size_t);
  int    (*seek) (void *, off_t, int);
};

struct fortran_var
{
  int n;
  int type;
  
  void *buf;
};

struct fortran_stream *fortran_stream_new (void *, 
                    size_t (*) (void *, void *, size_t),
                    int    (*) (void *, off_t, int));
                    
size_t fortran_read (struct fortran_stream *, void *, size_t);
int    fortran_seek (struct fortran_stream *, off_t, int);
void   fortran_stream_destroy (struct fortran_stream *);
void   fortran_stream_fclose (struct fortran_stream *);
struct fortran_stream *fortran_stream_fopen (const char *);
struct fortran_stream *fortran_stream_from_fp (FILE *fp);

int  fortran_getchar (struct fortran_stream *);
int  fortran_rew (struct fortran_stream *);

/* TODO: Homogenize this */
int  fortran_parse_format (char *, struct fortran_format *);
void fortran_format_destroy (struct fortran_format *);

struct fortran_var *
fortran_scan (struct fortran_stream *, struct fortran_format *, int);

void fortran_var_destroy (struct fortran_var *);

#endif /* _FORTRAN_H */

