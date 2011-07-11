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

struct strlist *
tagfile_parse_from_fp (FILE *fp)
{
  char *line, *tmp;
  struct strlist *list;
  
  list = strlist_new ();
  
  while ((tmp = fread_line (fp)) != NULL)
  {
    line = trim (tmp);
    free (tmp);
    strlist_append_string (list, line);
    free (line);
  }
  
  return list;
}

struct strlist *
tagfile_parse (const char *path)
{
  FILE *fp;
  struct strlist *result;
  
  if ((fp = fopen (path, "r")) == NULL)
    return NULL;
    
  result = tagfile_parse_from_fp (fp);
  
  fclose (fp);
  
  return result;
}


