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
#include <string.h>
#include <stdlib.h>

#include <math.h>

#include "vector.h"
#include "util.h"

#define ASSERT_SAMESIZE(v1, v2)                                        \
  if (v1->dimension != v1->dimension)                                  \
  {                                                                    \
    fprintf (stderr, "%s: vector dimensions don't macht\n", __func__); \
    abort ();                                                          \
  }

#define ASSERT_3D(v1)                                                  \
  if (v1->dimension != 3)                                              \
  {                                                                    \
    fprintf (stderr, "%s: vector is not tri-dimensional\n", __func__); \
    abort ();                                                          \
  }
  
void
vector_init (vector_t *vec, int dimension)
{
  void *base;
  vectcoef_t *coefs;
  
  base = xmalloc (sizeof (vectcoef_t) * (dimension) + CACHE_LINE_SIZE - 1);
  
  coefs = (vectcoef_t *) 
    (((unsigned long) base + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
  
  vec->dimension = dimension;
  vec->base = base;
  vec->coef = coefs;
}

void
vector_free (vector_t *vec)
{
  free (vec->base);
}

void
vector_add (vector_t *dest, const vector_t *orig)
{
  int i, siz;
  
  ASSERT_SAMESIZE (dest, orig);
  
  siz = orig->dimension;
  
  for (i = 0; i < siz; i++)
    dest->coef[i] += orig->coef[i];
}

void
vector_sub (vector_t *dest, const vector_t *orig)
{
  int i, siz;
  
  ASSERT_SAMESIZE (dest, orig);
  
  siz = orig->dimension;
  
  for (i = 0; i < siz; i++)
    dest->coef[i] -= orig->coef[i];
}

void
vector_sub_to (vector_t *dest, const vector_t *v1, const vector_t *v2)
{
  int i, siz;
  
  ASSERT_SAMESIZE (dest, v2);
  ASSERT_SAMESIZE (v1, v2);
  
  siz = v1->dimension;
  
  for (i = 0; i < siz; i++)
    dest->coef[i] = v1->coef[i] - v2->coef[i];
}

void
vector_add_ponderated (vector_t *dest, const vector_t *orig, vectcoef_t f)
{
  int i, siz;
  
  ASSERT_SAMESIZE (dest, orig);
  
  siz = orig->dimension;
  
  for (i = 0; i < siz; i++)
    dest->coef[i] += f * orig->coef[i];
}

void
vector_mul_by_const (vector_t *dest, vectcoef_t f)
{
  int i, siz;
  
  siz = dest->dimension;
  
  for (i = 0; i < siz; i++)
    dest->coef[i] *= f;
}

void
vector_mul_by_const_to (vector_t *dest, const vector_t *orig, vectcoef_t f)
{
  int i, siz;
  
  ASSERT_SAMESIZE (dest, orig);
  
  siz = dest->dimension;
  
  for (i = 0; i < siz; i++)
    dest->coef[i] = f * orig->coef[i];
}

void
vector_zero (vector_t *dest)
{
  int i;
  int siz;
  
  siz = dest->dimension;
  
  for (i = 0; i < siz; i++)
    dest->coef[i] = 0.0;
}

void
vector_copy (vector_t *dest, const vector_t *orig)
{
  int i;
  int siz;
  
  ASSERT_SAMESIZE (dest, orig);
  
  siz = dest->dimension;
  
  for (i = 0; i < siz; i++)
    dest->coef[i] = orig->coef[i];
}

vectcoef_t
vector_point_product (const vector_t *v1, const vector_t *v2)
{
  int i;
  int siz;
  
  vectcoef_t result = 0.0;
  
  ASSERT_SAMESIZE (v1, v2);
  
  siz = v1->dimension;
  
  for (i = 0; i < siz; i++)
    result += v1->coef[i] * v2->coef[i];
    
  return result;
}

/*
   | i  j  k  |
   | x1 y1 z1 | = (y1 * z2 - z1 * y2, z1 * x2 - x1 * z2, x1 * y2 - y1 * z2); 
   | x2 y2 z2 |
*/

void
vector_cross_product (vector_t *dest, const vector_t *v1, const vector_t *v2)
{
  ASSERT_SAMESIZE (dest, v1);
  ASSERT_SAMESIZE (dest, v2);
  ASSERT_3D (v1);
  
  dest->coef[0] = v1->coef[1] * v2->coef[2] - v1->coef[2] * v2->coef[1];
  dest->coef[1] = v1->coef[2] * v2->coef[0] - v1->coef[0] * v2->coef[2];
  dest->coef[2] = v1->coef[0] * v2->coef[1] - v1->coef[1] * v2->coef[0];
}

void
vector_cross_product_ponderated (vector_t *dest, 
                                 const vector_t *v1,
                                 const vector_t *v2,
                                 vectcoef_t f)
{
  ASSERT_SAMESIZE (dest, v1);
  ASSERT_SAMESIZE (dest, v2);
  ASSERT_3D (v1);
  
  dest->coef[0] = f * (v1->coef[1] * v2->coef[2] - v1->coef[2] * v2->coef[1]);
  dest->coef[1] = f * (v1->coef[2] * v2->coef[0] - v1->coef[0] * v2->coef[2]);
  dest->coef[2] = f * (v1->coef[0] * v2->coef[1] - v1->coef[1] * v2->coef[0]);
}

void 
vector_print (FILE *dest, vector_t *vec)
{
  int i;
  
  fputc ('(', dest);
  
  for (i = 0; i < vec->dimension; i++)
  {
    if (i)
      fprintf (dest, ", ");
    fprintf (dest, "%lg", vec->coef[i]);
  }
  
  fputc (')', dest);
}

vectcoef_t
vector_norm (const vector_t *v1)
{
  int i, siz;
  
  vectcoef_t result = 0.0;
  
  siz = v1->dimension;
  
  for (i = 0; i < siz; i++)
    result += v1->coef[i] * v1->coef[i];
  
  return sqrt (result);
}

vectcoef_t
vector_eucdist (const vector_t *v1, const vector_t *v2)
{
  int i, siz;
  
  vectcoef_t result = 0.0;
  
  ASSERT_SAMESIZE (v1, v2);
  
  siz = v1->dimension;
  
  for (i = 0; i < siz; i++)
    result += (v1->coef[i] - v2->coef[i]) * (v1->coef[i] - v2->coef[i]);
  
  return sqrt (result);
}

