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

#ifndef _JIT_H
#define _JIT_H

#include "util.h"

struct jit_code
{
  void *base;
  int size;
  unsigned long ptr;
};

struct jit_code *jit_code_new (int);
int jit_compile (struct jit_code *, struct decision_tree *);
struct output* jit_code_exec (struct jit_code *);
void jit_code_destroy (struct jit_code *);
#endif /* _JIT_H */

