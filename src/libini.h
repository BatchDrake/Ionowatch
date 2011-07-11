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


#ifndef _LIBINI_H
#define _LIBINI_H

#define LE_LF 0
#define LE_CRLF 1

#define mem_file_set_unix(file) file->fc_line_end = LE_LF
#define mem_file_set_dos(file) file->fc_line_end = LE_CRLF

typedef unsigned int line_t;

typedef struct _file_contents
{
  char **fc_lines;
  line_t fc_line_count;
  char  *fc_name;
  int    fc_line_end;
}
file_t;

#define IV_IS_FREE 0
#define IV_IS_USED 1
#define IV_IS_COMM 2
#define IV_IS_DATA 3

typedef struct _inipair
{
  int   ip_type;
  char *ip_key;
  char *ip_value;
}
inivalue_t;

typedef struct _inisection
{
  char  *is_name;
  int    is_value_count;
  int    is_used;
  inivalue_t *is_values;
}
ini_section_t;

typedef struct _inifile
{
  ini_section_t *if_sects;
  int      if_sect_count;
}
ini_t;

char *ini_value_lookup (ini_t *ini, char *section, char *key);
ini_section_t* ini_section_lookup (ini_t *ini, char *section);
int flush_mem_file (file_t* file);
int ini_file_dump (ini_t *ini, char *filedump);
int ini_load_from_file (char *path, ini_t *ini);
int ini_section_add_comment (ini_section_t *sect, char *comment);
int ini_section_add_pair (ini_section_t *sect, char *key, char *value);
int ini_section_add_data (ini_section_t *sect, char *data);
int ini_section_register (ini_t *ini, char *name);
int init_mem_file_blank (file_t *file, char *name);
int ini_value_drop (ini_t *ini, char *section, char *key);
int ini_value_edit (ini_t *ini, char *section, char *key, char *val);
int load_mem_file (char *path, file_t *dest);
int mem_file_add_line (file_t *dest, char *line);
int mem_file_insert (file_t *file, int line, char *text);
int mem_file_line_delete (file_t *file, int line);
void clear_mem_file (file_t *file);
void ini_file_destroy (ini_t *ini);
void ini_section_drop (ini_section_t *sect);

#endif /* _LIBINI_H */
