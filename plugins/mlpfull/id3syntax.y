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

%{
#define YYSTYPE void *
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "id3.h"

struct decision_tree *temp_tree;
char *expression_buffer;
int   expression_ptr;

void yyerror (const char *);
static int yylex (void);

%}

%start root_decision
%token EOL
%token IF
%token THEN
%token ELSE
%token OPENING_BRACKET
%token CLOSING_BRACKET

%token NUMERIC
%token TOKEN
%token GTE

%%

root_decision : 
         EOL                 { $$ = temp_tree; }
       | decision            { $$ = temp_tree; 
                               decision_tree_set_root (temp_tree, $1); }
       | root_decision EOL   { $$ = $1; }
       
;

decision :
        IF OPENING_BRACKET TOKEN GTE NUMERIC THEN decision ELSE decision CLOSING_BRACKET
                             { $$ = threshold_node_new (input_param_lookup_autoalloc (temp_tree, $3), *(float *) $5);
                               ((struct node *) $$)->true_node  = $7;
                               ((struct node *) $$)->false_node = $9;
                               
                               free ($5);
                               free ($3);
                             }
                             
       | TOKEN               { $$ = leaf_node_new (output_lookup_autoalloc (temp_tree, $1)); 
                               free ($1);
                             }
;


%%

#ifdef yygetc
  #undef yygetc
#endif

void 
id3_set_expression (char *expr)
{
  expression_buffer = expr; /* BE CAREFUL, SERIOUSLY */
  temp_tree = decision_tree_new ();
}

int
id3_parse (void)
{
  return yyparse ();
}

struct decision_tree *
id3_get_tree (void)
{
  return temp_tree;
}



int
yygetc (void)
{
  if (expression_ptr == strlen (expression_buffer))
    return EOF;

  return expression_buffer[expression_ptr++];
}

int
yyungetc (int c)
{
  if (expression_ptr)
    expression_buffer[--expression_ptr] = c;

  return 0;
}

void
yyerror (const char *error)
{
  fprintf (stderr, "yyerror: %s at buffer byte %d\n", error, expression_ptr);
  
  exit (1);
}

int
yylex (void)
{
  char c;
  int len, n;
  char *buffer;
  float *numbuf;
  
  len = 10;
  n = 0;
  
reparse:
  if ((c = yygetc ()) == EOF)
    return 0;

  switch (c)
  {
    case ' ' :
    case '\t':
    case '\r':
      goto reparse;
    
    case '\n':
      return EOL;
      
    case '{':
      return OPENING_BRACKET;
    
    case '}':
      return CLOSING_BRACKET;
    
    case '-':
      break;
      
    case '>':
      if ((c = yygetc ()) != '=')
        return -1;
        
      return GTE;
  }

  if (isdigit (c) || c == '-')
  {
    buffer = xmalloc (len + 1);
    buffer[n++] = c;

    /* TODO: buena idea abstraer esto. */
    while (((c = yygetc ()) != EOF) && (isdigit (c) || (c == 'e') || (c == '.') || (c == 'E') || (c == '-')))
    {
      if (n == len)
      {
        len <<= 1;
        buffer = xrealloc (buffer, len + 1);   
      }
      
      buffer[n++] = c;
    }
    
    yyungetc (c);
    
    buffer[n] = 0;
    
    numbuf = xmalloc (sizeof (float));
    
    if (!sscanf (buffer, "%f", numbuf))
    {
      fprintf (stderr, "error: constante (%s) incorrecta\n", buffer);
      free (buffer);
      free (numbuf);
      return -1;
    }
    
    yylval = numbuf;
    free (buffer);
    return NUMERIC;
    
  }
  else if (isalpha (c))
  {
    buffer = xmalloc (len + 1);
    buffer[n++] = c;

    /* TODO: buena idea abstraer esto. */
    while (((c = yygetc ()) != EOF) && (isalnum (c) || c == '_'))
    {
      if (n == len)
      {
        len <<= 1;
        buffer = xrealloc (buffer, len + 1);   
      }
      
      buffer[n++] = c;
    }
    
    yyungetc (c);
    
    buffer[n] = 0;
    
    if (strcmp (buffer, "if") == 0)
    {
      free (buffer);
      return IF;
    }
    else if (strcmp (buffer, "then") == 0)
    {
      free (buffer);
      return THEN;
    }
    else if (strcmp (buffer, "else") == 0)
    {
      free (buffer);
      return ELSE;
    }
    
    yylval = buffer;
    
    return TOKEN; /* Immediate */
  }
  
  
  printf ("Character %d (%c) unknown\n", c, c);
  return -1;

}

