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

#ifndef _TREE_H
#define _TREE_H

#define NODE_TYPE_THRESHOLD 0
#define NODE_TYPE_LEAF      1

#include "util.h"

struct input_param
{
  char *name;
  float value;
};

struct output 
{
  char *name;
  int   number;
};

struct node
{
  int                 type;
  
  struct output      *output;
  struct input_param *param;
  float               threshold;
  
  struct              node *true_node, *false_node;
};

struct decision_tree
{
  struct node *root;
  
  PTR_LIST (struct input_param, input_param);
  PTR_LIST (struct output, output);
};

struct decision_tree *decision_tree_new (void);
void decision_tree_set_root (struct decision_tree *, struct node *);
void decision_tree_destroy (struct decision_tree *);

struct node *threshold_node_new (struct input_param *, float);
struct node *leaf_node_new (struct output *);
struct output *output_lookup_autoalloc (struct decision_tree *, const char *);
struct input_param *input_param_lookup_autoalloc (struct decision_tree *, const char *);

#endif /* _TREE_H */

