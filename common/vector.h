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

#ifndef _VECTOR_H
#define _VECTOR_H

#ifndef CACHE_LINE_SIZE
# define CACHE_LINE_SIZE 64 /* Cambiar en función de la CPU */
#endif

typedef double vectcoef_t;

struct vector
{
  int dimension;
  void *base;
  vectcoef_t *coef;
} __attribute__ ((aligned (CACHE_LINE_SIZE)));

typedef struct vector vector_t;

void vector_init (vector_t *, int);
void vector_free (vector_t *);
void vector_add (vector_t *, const vector_t *);
void vector_sub (vector_t *, const vector_t *);
void vector_sub_to (vector_t *, const vector_t *, const vector_t *);

void vector_add_ponderated (vector_t *, const vector_t *, vectcoef_t);
void vector_mul_by_const (vector_t *, vectcoef_t);
void vector_mul_by_const_to (vector_t *, const vector_t *, vectcoef_t);
void vector_zero (vector_t *);
void vector_copy (vector_t *, const vector_t *);

vectcoef_t vector_point_product (const vector_t *, const vector_t *);
void vector_cross_product (vector_t *, const vector_t *, const vector_t *);
void vector_cross_product_ponderated (vector_t *,
                                       const vector_t *,
                                       const vector_t *,
                                       vectcoef_t);
                                       
void vector_print (FILE *, vector_t *);
vectcoef_t vector_norm (const vector_t *);
vectcoef_t vector_eucdist (const vector_t *, const vector_t *);
#endif /* _VECTOR_H */

