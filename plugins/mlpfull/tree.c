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

#include "util.h"
#include "tree.h"

struct input_param *
input_param_new (const char *name)
{
  struct input_param *new;
  
  new = xmalloc (sizeof (struct input_param));
  
  memset (new, 0, sizeof (struct input_param));
  
  new->name = xstrdup (name);
  
  return new;
}

struct output *
output_new (const char *name)
{
  struct output *new;
  
  new = xmalloc (sizeof (struct output));
  
  memset (new, 0, sizeof (struct output));
  
  new->name = xstrdup (name);
  
  return new;
}

struct node *
leaf_node_new (struct output *output)
{
  struct node *new;
  
  new = xmalloc (sizeof (struct node));
  
  memset (new, 0, sizeof (struct node));
  
  new->type = NODE_TYPE_LEAF;

  new->output = output;  
  return new;
}

struct node *
threshold_node_new (struct input_param *param, float threshold)
{
  struct node *new;
  
  new = xmalloc (sizeof (struct node));
  
  memset (new, 0, sizeof (struct node));
  
  new->type = NODE_TYPE_THRESHOLD;

  new->param = param;
  new->threshold = threshold;

  return new;
}

struct decision_tree *
decision_tree_new (void)
{
  struct decision_tree *new;
  
  new = xmalloc (sizeof (struct decision_tree));
  
  memset (new, 0, sizeof (struct decision_tree));
  
  return new;
}

void
input_param_destroy (struct input_param *param)
{
  free (param->name);
  free (param);
}

void
output_destroy (struct output *output)
{
  free (output->name);
  free (output);
}

void
node_destroy (struct node *node)
{
  switch (node->type)
  {
/* TODO: if node depth is way too high, we should make a node pointer list
   and destroy one by one iterating that list, we don't want a stack
   overflow here, right? */
    case NODE_TYPE_THRESHOLD:
      node_destroy (node->true_node);
      node_destroy (node->false_node);
    
    case NODE_TYPE_LEAF:
      break;
      
    default:
      ERROR ("unknown node type %d (corruption?)\n", node->type);
      abort ();
  }
  
  free (node);
}

void
node_debug (struct node *node)
{
  switch (node->type)
  {
/* TODO: if node depth is way too high, we should make a node pointer list
   and destroy one by one iterating that list, we don't want a stack
   overflow here, right? */
    case NODE_TYPE_THRESHOLD:
      printf ("if (%s >= %g) { ", node->param->name, node->threshold);
      
      node_debug (node->true_node);
      
      printf (" } else { ");
      
      node_debug (node->false_node);
    
      printf (" }");
      break;
      
    case NODE_TYPE_LEAF:
      printf ("return %s", node->output->name);
      break;
      
    default:
      ERROR ("unknown node type %d (corruption?)\n", node->type);
      abort ();
  }
  
}
void
decision_tree_destroy (struct decision_tree *tree)
{
  int i;
  
  if (tree->root != NULL)
    node_destroy (tree->root);
    
  for (i = 0; i < tree->input_param_count; i++)
    if (tree->input_param_list[i] != NULL)
      input_param_destroy (tree->input_param_list[i]);
 
  if (tree->input_param_list != NULL)
    free (tree->input_param_list);
        
  for (i = 0; i < tree->output_count; i++)
    if (tree->output_list[i] != NULL)
      output_destroy (tree->output_list[i]);
      
  if (tree->output_list != NULL)
    free (tree->output_list);
  
  free (tree);
}

void
decision_tree_set_root (struct decision_tree *tree, struct node *root)
{
  tree->root = root;
}


struct input_param *
input_param_lookup (struct decision_tree *tree, const char *name)
{
  int i;

  for (i = 0; i < tree->input_param_count; i++)
    if (tree->input_param_list[i] != NULL)
      if (strcmp (tree->input_param_list[i]->name, name) == 0)
        return tree->input_param_list[i];
 
  return NULL;
}

struct output *
output_lookup (struct decision_tree *tree, const char *name)
{
  int i;

  for (i = 0; i < tree->output_count; i++)
    if (tree->output_list[i] != NULL)
      if (strcmp (tree->output_list[i]->name, name) == 0)
        return tree->output_list[i];
 
  return NULL;
}

struct input_param *
input_param_lookup_autoalloc (struct decision_tree *tree, const char *name)
{
  struct input_param *this;
  
  if ((this = input_param_lookup (tree, name)) == NULL)
  {
    this = input_param_new (name);
    PTR_LIST_APPEND (tree->input_param, this);
  }
  
  return this;
}

struct output *
output_lookup_autoalloc (struct decision_tree *tree, const char *name)
{
  struct output *this;
  
  if ((this = output_lookup (tree, name)) == NULL)
  {
    this = output_new (name);
    PTR_LIST_APPEND (tree->output, this);
  }
  
  return this;
}

