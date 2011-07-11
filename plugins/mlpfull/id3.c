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

#include "id3.h"

struct id3_parser *
id3_parser_new_nojit_from_tagfile (struct strlist *tagfile, FILE *expr_fp)
{
  struct decision_tree *tree;
  struct id3_parser *parser;
  
  char *expression_buffer;
  char c;
  int size, p;
  int i, j, found;
  
  size = 1;
  p = 0;
  
  expression_buffer = xstrdup ("");
  
  while ((c = fgetc (expr_fp)) != EOF)
  {
    if (p == size)
    {
      size <<= 1;
      expression_buffer = xrealloc (expression_buffer, size + 2);
      expression_buffer[size] = '\0';
    }
    
    expression_buffer[p++] = c;
    expression_buffer[p] = '\0';
  }
  
  id3_set_expression (expression_buffer);
  
  if (id3_parse () < 0)
  {
    ERROR ("code parsing failed\n");
    free (expression_buffer);
    
    return NULL;
  }

  /* We don't need this anymore */
  
  free (expression_buffer);
    
  tree = id3_get_tree ();
  
  parser = xmalloc (sizeof (struct id3_parser));
  
  parser->indices = xmalloc (tree->input_param_count * sizeof (int));
  parser->field_names = tagfile;
  parser->tree = tree;
  parser->code = NULL;
  
  for (j = 0; j < tree->input_param_count; j++)
    if (tree->input_param_list[j] != NULL)
    {
      found = 0;
      
      for (i = 0; i < tagfile->strings_count; i++)
        if (tagfile->strings_list[i] != NULL)
          if (strcmp (tree->input_param_list[j]->name, 
                      tagfile->strings_list[i]) == 0)
          {
            parser->indices[j] = i;
            found++;
            break;
          }
          
      if (!found)
      {
        ERROR ("param %s not found in tag list\n", 
          tree->input_param_list[j]->name);
          
        id3_parser_destroy (parser);
        
        return NULL;
      }
    }
  

  
  return parser;
}

struct id3_parser *
id3_parser_new_from_tagfile (struct strlist *tagfile, FILE *expr_fp)
{
  struct id3_parser *parser;
  
  if ((parser = id3_parser_new_nojit_from_tagfile (tagfile, expr_fp)) == NULL)
    return NULL;
    
  if ((parser->code = jit_code_new (4096)) == NULL)
    NOTICE ("no JIT engine enabled for this architecture, emulating behavior\n");
  else if (jit_compile (parser->code, parser->tree) == -1)
  {
    ERROR ("JIT compilation error\n");
    id3_parser_destroy (parser);
    return NULL;
  }
  
  return parser;
}

void
id3_parser_destroy (struct id3_parser *parser)
{
  decision_tree_destroy (parser->tree);
  strlist_destroy (parser->field_names);
  
  free (parser->indices);
  
  if (parser->code != NULL)
    jit_code_destroy (parser->code);
    
  free (parser);
}

static inline struct output *
__id3_eval_recursive (struct decision_tree *tree, struct node *node)
{
  switch (node->type)
  {
    case NODE_TYPE_LEAF:
      return node->output;
      
    case NODE_TYPE_THRESHOLD:
      if (node->param->value >= node->threshold)
        return __id3_eval_recursive (tree, node->true_node);
      else
        return __id3_eval_recursive (tree, node->false_node);
    
    default:
      ERROR ("unknown node type (corruption)\n");
      abort ();
  }
  
  return NULL; /* This will never happen */
}

struct output *
id3_eval (struct id3_parser *parser, float *vector)
{
  int i;
  
  for (i = 0; i < parser->tree->input_param_count; i++)
    parser->tree->input_param_list[i]->value = vector[parser->indices[i]];
  
  if (parser->code == NULL)
    return __id3_eval_recursive (parser->tree, parser->tree->root);
  else
    return jit_code_exec (parser->code);
}

