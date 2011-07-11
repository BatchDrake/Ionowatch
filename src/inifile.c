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


#include "libini.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

static int __setup_blank_section (ini_section_t *sect, char *name)
{
  sect->is_name = xstrdup (name);
  
  sect->is_value_count = 0;
  sect->is_used = 1;
  sect->is_values = NULL;
  
  return 0;
}

int ini_section_register (ini_t *ini, char *name)
{
  void *p;
  int i;
  
  for (i = 0; i < ini->if_sect_count; i++)
  {
    if (!ini->if_sects[i].is_used)
    {
      if (__setup_blank_section (&ini->if_sects[i], name) == -1)
        return -1;
      
      return i;
    }
  }
  
  p = xrealloc (ini->if_sects, (ini->if_sect_count + 1) * sizeof (ini_section_t));
  
  ini->if_sects = (ini_section_t *) p;

  if (__setup_blank_section (&ini->if_sects[ini->if_sect_count], name) == -1)
    return -1;
  
  return ini->if_sect_count++;
}

int ini_section_add_pair (ini_section_t *sect, char *key, char *value)
{
  char *kdup, *vdup;
  int i;
  void *p;
  
  if (*key == '"' && key[strlen (key) - 1] == '"')
  {
    key[strlen (key) - 1] = '\0';
    key++;
  }
  
  if (*value == '"' && value[strlen (value) - 1] == '"')
  {
    value[strlen (value) - 1] = '\0';
    value++;
  }
  
  
  kdup = xstrdup (key);
  vdup = xstrdup (value);
  
  for (i = 0; i < sect->is_value_count; i++)
  {
    if (sect->is_values[i].ip_type == IV_IS_FREE)
    {
      sect->is_values[i].ip_key = kdup;
      sect->is_values[i].ip_value = vdup;
      sect->is_values[i].ip_type = IV_IS_USED;
      return i;
    }
  }
  
  p = xrealloc (sect->is_values, sizeof (inivalue_t) * (sect->is_value_count + 1));
  
  sect->is_values = (inivalue_t *) p;
  
  sect->is_values[sect->is_value_count].ip_key = kdup;
  sect->is_values[sect->is_value_count].ip_value = vdup;
  sect->is_values[sect->is_value_count].ip_type = IV_IS_USED;
  
  return sect->is_value_count++;
}

int ini_section_add_comment (ini_section_t *sect, char *comment)
{
  char *cdup;
  int i;
  void *p;
  cdup = xstrdup (comment);
  
  for (i = 0; i < sect->is_value_count; i++)
  {
    if (sect->is_values[i].ip_type == IV_IS_FREE)
    {
      sect->is_values[i].ip_key = NULL;
      sect->is_values[i].ip_value = cdup;
      sect->is_values[i].ip_type = IV_IS_COMM;
      return i;
    }
  }
  
  p = xrealloc (sect->is_values, sizeof (inivalue_t) * (sect->is_value_count + 1));
  sect->is_values = (inivalue_t *) p;
  
  sect->is_values[sect->is_value_count].ip_key = NULL;
  sect->is_values[sect->is_value_count].ip_value = cdup;
  sect->is_values[sect->is_value_count].ip_type = IV_IS_COMM;
  
  return sect->is_value_count++;
}


int ini_section_add_data (ini_section_t *sect, char *data)
{
  char *ddup;
  int i;
  void *p;
  
  if (*data == '"' && data[strlen (data) - 1] == '"')
  {
    data[strlen (data) - 1] = '\0';
    data++;
  }
  
  ddup = xstrdup (data);
  
  for (i = 0; i < strlen (ddup); i++)
    if (ddup[i] == '\x7') /* Este caracter no vale para nada */
      ddup[i] = '=';
  
  for (i = 0; i < sect->is_value_count; i++)
  {
    if (sect->is_values[i].ip_type == IV_IS_FREE)
    {
      sect->is_values[i].ip_key = NULL;
      sect->is_values[i].ip_value = ddup;
      sect->is_values[i].ip_type = IV_IS_DATA;
      return i;
    }
  }
  
  p = xrealloc (sect->is_values, sizeof (inivalue_t) * (sect->is_value_count + 1));
  sect->is_values = (inivalue_t *) p;
  
  sect->is_values[sect->is_value_count].ip_key = NULL;
  sect->is_values[sect->is_value_count].ip_value = ddup;
  sect->is_values[sect->is_value_count].ip_type = IV_IS_DATA;
  
  return sect->is_value_count++;
}

void ini_section_drop (ini_section_t *sect)
{
  int i;
  
  free (sect->is_name);
  sect->is_used = 0;
  
  for (i = 0; i < sect->is_value_count; i++)
  {
    free (sect->is_values[i].ip_key);
    free (sect->is_values[i].ip_value);
  }
  
  free (sect->is_values);
}

ini_section_t* ini_section_lookup (ini_t *ini, char *section)
{
  int i;
  
  for (i = 0; i < ini->if_sect_count; i++)
    if (ini->if_sects[i].is_used)
      if (strcasecmp (ini->if_sects[i].is_name, section) == 0)
        return &ini->if_sects[i];
  
  return NULL;
}

char *ini_value_lookup (ini_t *ini, char *section, char *key)
{
  ini_section_t *sect;
  int i;
  
  sect = ini_section_lookup (ini, section);
  
  if (sect == NULL)
    return NULL;
  
  for (i = 0; i < sect->is_value_count; i++)
    if (sect->is_values[i].ip_type == IV_IS_USED)
      if (strcasecmp (sect->is_values[i].ip_key, key) == 0)
        return sect->is_values[i].ip_value;
  
  return NULL;
}

int ini_value_edit (ini_t *ini, char *section, char *key, char *val)
{
  ini_section_t *sect;
  int i;
  char *vdup;
  
  vdup = xstrdup (val);
  
  sect = ini_section_lookup (ini, section);
  
  if (sect == NULL)
    return -1;
  
  for (i = 0; i < sect->is_value_count; i++)
    if (sect->is_values[i].ip_type == IV_IS_USED)
      if (strcasecmp (sect->is_values[i].ip_key, key) == 0)
      {
        free (sect->is_values[i].ip_value);
        sect->is_values[i].ip_value = vdup;
        return 0;
      }
  
  return -1;
}


int ini_value_drop (ini_t *ini, char *section, char *key)
{
  ini_section_t *sect;
  int i;
  
  sect = ini_section_lookup (ini, section);
  
  if (sect == NULL)
    return -1;
  
  for (i = 0; i < sect->is_value_count; i++)
    if (sect->is_values[i].ip_type == IV_IS_USED)
      if (strcasecmp (sect->is_values[i].ip_key, key) == 0)
      {
        sect->is_values[i].ip_type = IV_IS_FREE;
        free (sect->is_values[i].ip_value);
        if (sect->is_values[i].ip_key)
          free (sect->is_values[i].ip_key);
        return 0;
      }
  
  return -1;
}

void ini_file_destroy (ini_t *ini)
{
  int i;
  
  for (i = 0; i < ini->if_sect_count; i++)
    ini_section_drop (&ini->if_sects[i]);
  
  ini->if_sect_count = 0;
  free (ini->if_sects);
  ini->if_sects = NULL;
}

int value_has_spaces (char *v)
{
  if (isspace (*v) || isspace (v[strlen (v) - 1]))
    return 1;
  
  return 0;
}

int ini_file_dump (ini_t *ini, char *filedump)
{
  FILE *fp;
  int i, j, n;
  
  fp = fopen (filedump, "w");
  
  if (fp == NULL)
    return -1;
  
  /* Lo primero es buscar la sección global */
  
  for (i = 0; i < ini->if_sect_count; i++)
    if (ini->if_sects[i].is_name[0] == '\0')
    {
      for (j = 0; j < ini->if_sects[i].is_value_count; j++)
      {
        if (ini->if_sects[i].is_values[j].ip_type == IV_IS_COMM)
          fprintf (fp, ";%s\n", ini->if_sects[i].is_values[j].ip_value);
        else if (ini->if_sects[i].is_values[j].ip_type == IV_IS_DATA)
        {
          for (n = 0; n < strlen (ini->if_sects[i].is_values[j].ip_value); n++)
            if (ini->if_sects[i].is_values[j].ip_value[n] == '=')
              ini->if_sects[i].is_values[j].ip_value[n] = '\x7';
          if (value_has_spaces (ini->if_sects[i].is_values[j].ip_value))
            fprintf (fp, "\"%s\"\n", ini->if_sects[i].is_values[j].ip_value);
          else
            fprintf (fp, "%s\n", ini->if_sects[i].is_values[j].ip_value);
        }
        else if (ini->if_sects[i].is_values[j].ip_type == IV_IS_USED)
        {
          if (value_has_spaces (ini->if_sects[i].is_values[j].ip_key))
            fprintf (fp, "\"%s\"", ini->if_sects[i].is_values[j].ip_key);
          else
            fprintf (fp, "%s", ini->if_sects[i].is_values[j].ip_key);
          
          if (value_has_spaces (ini->if_sects[i].is_values[j].ip_value))
            fprintf (fp, " = \"%s\"\n", ini->if_sects[i].is_values[j].ip_value);
          else
            fprintf (fp, " = %s\n", ini->if_sects[i].is_values[j].ip_value);
        }
      }
      
      fprintf (fp, "\n");
      break;
    }
  
  for (i = 0; i < ini->if_sect_count; i++)
    if (ini->if_sects[i].is_name[0])
    {
      fprintf (fp, "[%s]\n", ini->if_sects[i].is_name);
      for (j = 0; j < ini->if_sects[i].is_value_count; j++)
      {
        if (ini->if_sects[i].is_values[j].ip_type == IV_IS_COMM)
          fprintf (fp, ";%s\n", ini->if_sects[i].is_values[j].ip_value);
        else if (ini->if_sects[i].is_values[j].ip_type == IV_IS_DATA)
        {
          for (n = 0; n < strlen (ini->if_sects[i].is_values[j].ip_value); n++)
            if (ini->if_sects[i].is_values[j].ip_value[n] == '=')
              ini->if_sects[i].is_values[j].ip_value[n] = '\x7';
          
          if (value_has_spaces (ini->if_sects[i].is_values[j].ip_value))
            fprintf (fp, "\"%s\"\n", ini->if_sects[i].is_values[j].ip_value);
          else
            fprintf (fp, "%s\n", ini->if_sects[i].is_values[j].ip_value);
        }
        else if (ini->if_sects[i].is_values[j].ip_type == IV_IS_USED)
        {
          if (value_has_spaces (ini->if_sects[i].is_values[j].ip_key))
            fprintf (fp, "\"%s\"", ini->if_sects[i].is_values[j].ip_key);
          else
            fprintf (fp, "%s", ini->if_sects[i].is_values[j].ip_key);
          
          if (value_has_spaces (ini->if_sects[i].is_values[j].ip_value))
            fprintf (fp, " = \"%s\"\n", ini->if_sects[i].is_values[j].ip_value);
          else
            fprintf (fp, " = %s\n", ini->if_sects[i].is_values[j].ip_value);
        }
      }
      fprintf (fp, "\n");
    }
  
  fclose (fp);
  
  return 0;
}

int ini_load_from_file (char *path, ini_t *ini)
{
  file_t file;
  char *buf;
  char *lcopy;
  
  char *keyptr;
  char *valptr;
  
  int i, j, n;
  int sid;
  int s;
  int eq_found;
  
  ini_section_t *sect = NULL;
  
  memset (ini, 0, sizeof (ini_t));
  
  
  if (load_mem_file (path, &file) == -1)
    return -1;

  for (i = 0; i < file.fc_line_count; i++)
  {
    buf = file.fc_lines[i];
    while (isspace (*buf) && *buf)
      buf++;
    
    if (!*buf)
      continue;
    
    s = strlen (buf);
    for (n = (s - 1); n >= 0; n--)
      if (!isspace (buf[n]))
        break;
    
    lcopy = xstrdup (buf);
    
    lcopy[n + 1] = '\0';
    
    buf = lcopy;
    s = strlen (buf);
    
    if (buf[0] == '[')
    {
      /* Esto es una sección */
      buf[s - 1] = '\0';
      sid = ini_section_register (ini, buf + 1);
      if (sid == -1)
      {
        free (lcopy);
        ini_file_destroy (ini);
        return -1;
      }
      
      sect = &ini->if_sects[sid];
    }
    else if (buf[0] == ';')
    {
      if (!sect)
      {
        sid = ini_section_register (ini, ""); /* Creamos la sección global */
        if (sid == -1)
        {
          free (lcopy);
          ini_file_destroy (ini);
          return -1;
        }
        
        sect = &ini->if_sects[sid];
      }
      
      ini_section_add_comment (sect, buf + 1);
    }
    else
    {
      if (!sect)
      {
        sid = ini_section_register (ini, ""); /* Creamos la sección global */
        if (sid == -1)
        {
          free (lcopy);
          ini_file_destroy (ini);
          return -1;
        }
        
        sect = &ini->if_sects[sid];
      }
      
      eq_found = 0;
      
      for (j = 0; j < s; j++)
      {
        if (buf[j] == '=')
        {
          
          eq_found++;
          break;
        }
      }
      
      /* Datos. */
      if (!eq_found)
      {
        ini_section_add_data (sect, buf);
        free (lcopy);
        continue;
      }
      
      buf[j] = '\0';
      keyptr = buf;
      
      if (!*buf)
      {
        buf[j] = '=';
        ini_section_add_data (sect, buf);
        free (lcopy);
        continue;
      }
      
      valptr = &buf[j + 1];
      
      while (isspace (*valptr))
        valptr++;
      j--;
      
      while (isspace (buf[j]))
        buf[j--] = '\0';
      
      ini_section_add_pair (sect, keyptr, valptr);
    }
    
    free (lcopy);
  }
  
  return 0;
}
