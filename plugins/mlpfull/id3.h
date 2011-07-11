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

#ifndef _ID3_H
#define _ID3_H

#include "util.h"
#include "tree.h"
#include "datafile.h"
#include "jit.h"

struct id3_parser
{
  struct strlist *field_names;
  struct decision_tree *tree;
  int *indices;
  struct jit_code *code;
  
};

void id3_set_expression (char *);
int id3_parse (void);

struct decision_tree *id3_get_tree (void);
struct id3_parser *id3_parser_new_nojit (const char *, FILE *);
struct id3_parser *id3_parser_new_from_tagfile (struct strlist *, FILE *);
void id3_parser_destroy (struct id3_parser *);
struct output *id3_eval (struct id3_parser *, float *);

#endif /* _ID3_H */

