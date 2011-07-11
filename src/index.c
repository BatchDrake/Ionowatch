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
#include <time.h>
#include <libgen.h>
#include <sys/file.h>

#include <ionowatch.h>

PTR_LIST (struct ionogram_index, inventories);
PTR_LIST (struct ionogram_filetype, supported_filetypes);

struct ionogram_index *
ionogram_index_new (const char *name, const char *url)
{
  struct ionogram_index *new;
  
  new = xmalloc (sizeof (struct ionogram_index));
  
  memset (new, 0, sizeof (struct ionogram_index));
  
  new->name = xstrdup (name);
  new->url  = strbuild ("%s%c", url, url[strlen (url) - 1] == '/' ? 0 : '/');
  
  return new;
}

struct ionogram_filetype *
ionogram_filetype_new (char ext[4], char *desc,
                       int  (*parse_callback) (struct ionogram *, FILE *),
                       char *(*build_url_callback) (const struct ionogram_filename *, const char *))
{
  struct ionogram_filetype *new;
  
  new = xmalloc (sizeof (struct ionogram_filetype));
  
  memcpy (new->extension, ext, 3);
  
  new->extension[3] = 0; /* Just for being sure */
  
  new->desc = xstrdup (desc);
  new->parse_callback = parse_callback;
  new->build_url_callback = build_url_callback;
  
  return new;
}

void
ionogram_filetype_register (char ext[4], char *desc,
                            int  (*parse_callback) (struct ionogram *, FILE *),
                            char *(*build_url_callback) (const struct ionogram_filename *, const char *))
{
  struct ionogram_filetype *new;
  
  if (supported_filetypes_count >= sizeof (ionogram_bitmap_t) * 8)
  {
    ERROR ("too many registered filetypes\n");
    return;
  }
  
  new = ionogram_filetype_new (ext, desc, parse_callback, build_url_callback);
  
  PTR_LIST_APPEND (supported_filetypes, new);
}

struct ionogram_filetype *
ionogram_filetype_lookup (const char *ext)
{
  int i;
  
  for (i = 0; i < supported_filetypes_count; i++)
    if (supported_filetypes_list[i] != NULL)
      if (strcmp (ext, supported_filetypes_list[i]->extension) == 0)
      {
        supported_filetypes_list[i]->type = 1 << i; /* I know what I'm doing */
        return supported_filetypes_list[i];
      }
        
  return NULL;
}


static inline ionogram_bitmap_t
index_to_type (int index)
{
  return (1ULL << (ionogram_bitmap_t) index);
}


static int
type_to_index (ionogram_bitmap_t type)
{
  int i;
  
  for (i = 0; i < supported_filetypes_count; i++)
    if (type & (1ULL << (ionogram_bitmap_t) i))
      return i;
      
  return -1;
}

static ionogram_bitmap_t
types_in_common (ionogram_bitmap_t type1, ionogram_bitmap_t type2)
{
  return type1 & type2;
}

char *
ionogram_filename_str (struct ionogram_filename *info)
{
  int i;
  int index;
  
  index = type_to_index (info->type);
  
  if (index < 0 || index >= supported_filetypes_count)
  {
    ERROR ("invalid ionogram file type\n");
    return NULL;
  }
  
  
  return strbuild ("%5s_%04u%03u%02u%02u%02u.%3s",
    info->station,
    info->year,
    info->yday,
    info->h, info->m, info->s,
    supported_filetypes_list[index]->extension);
}

char *
ionogram_dayly_cache (const struct ionogram_filename *info)
{
  char *path;
  
  path = strbuild ("%s/%5s_%04u%03u.list", 
    get_ionowatch_cache_dir (), info->station, info->year, info->yday);

  return path;
}


int
ionogram_parse_filename (const char *filename, struct ionogram_filename *dst)
{
  char extension[4];
  int daymonth;
  struct ionogram_filetype *ft;
  struct tm tm;
  
  dst->station[5] = 0;
  extension[3] = 0;
  
  if (sscanf (filename, 
    "%5s_%04u%03u%02u%02u%02u.%3s", 
    dst->station, 
    &dst->year, 
    &dst->yday, 
    &dst->h, 
    &dst->m, 
    &dst->s, 
    extension) != 7)
      return -1;
      
  if ((ft = ionogram_filetype_lookup (extension)) == NULL)
    return -1;
    
  dst->type = ft->type;
  
  daymonth = yday_to_daymonth (dst->yday, dst->year);
  
  dst->day = daymonth & 31;
  dst->month = daymonth >> 5;
  
  memset (&tm, 0, sizeof (struct tm));
  
  tm.tm_sec   = dst->s;
  tm.tm_min   = dst->m;
  tm.tm_hour  = dst->h;
  tm.tm_mday  = dst->day + 1;
  tm.tm_mon   = dst->month;
  tm.tm_year  = dst->year - 1900;
  tm.tm_isdst = -1;
  
  dst->time = mktime (&tm);
  
  return 0;
}

void
load_ionograms_from_url (const char *url)
{
  struct strlist *ionograms;
  struct ionogram_filename fn;
  
  char *ionogram_path;
  char *ionogram_url;
  FILE *fp;
  int i;
  
  ionograms = parse_apache_index (url);
  
  /* Make these comparisons fancier, i.e have a list of supported extensions */
  for (i = 0; i < ionograms->strings_count; i++)
  {
    if (ionogram_parse_filename (ionograms->strings_list[i], &fn) != -1)
    {
      ionogram_path = strbuild ("%s/%s", 
        get_ionowatch_cache_dir (), ionograms->strings_list[i]);
      
      if (access (ionogram_path, F_OK) == -1)
      {
        fp = fopen (ionogram_path, "wb");
        
        if (fp == NULL)
        {
          ERROR ("couldn't create `%s': %s\n", ionogram_path, strerror (errno));
          
          free (ionogram_path);
          continue;
        }
        
        ionogram_url = strbuild ("%s%s", url, ionograms->strings_list[i]);
        
        (void) http_download (ionogram_url, fp);
        
        free (ionogram_url);
        fclose (fp);
      }
      
      free (ionogram_path);
    } 
  }
  
  strlist_destroy (ionograms);
}

/* TODO: use libcurl */

static int
download_dayly_list (const struct ionogram_filename *info, const char *cachefile)
{
  FILE *fp;
  int i, j, n;
  char *url;
  
  struct strlist *base, *new;
    
  base = strlist_new ();
  
  for (n = 0; n < supported_filetypes_count; n++)
  { 
    for (i = 0; i < inventories_count; i++)
      if (inventories_list[i] != NULL)
      {
        url = (supported_filetypes_list[n]->build_url_callback) 
          (info, inventories_list[i]->url);
          
        new = parse_apache_index (url);
        
        for (j = 0; j < new->strings_count; j++)
          if (new->strings_list[j] != NULL)
            if (strncmp (
              &new->strings_list[j][strlen (new->strings_list[j]) - 3],
                supported_filetypes_list[n]->extension, 3) == 0)
                strlist_append_string (base, new->strings_list[j]);
               
        strlist_destroy (new);
        free (url);
      }
  }
  
  if ((fp = fopen (cachefile, "wb")) == NULL)
  {
    strlist_destroy (base);
    return -1;
  }
  
  for (i = 0; i < base->strings_count; i++)
    if (base->strings_list[i] != NULL)
      fprintf (fp, "%s\n", base->strings_list[i]);
      
  fclose (fp);
  strlist_destroy (base);
  
  return 0;
}

/* TODO: cache this */
struct strlist *
get_day_ionograms (const struct ionogram_filename *info)
{
  int n;
  char *cachefile;
  char fileline[RECOMMENDED_LINE_SIZE];
  char *p;
  FILE *fp;
  
  struct strlist *base;
  struct strlist *new;
  
  base = strlist_new ();
  
  cachefile = ionogram_dayly_cache (info);
  
  if (access (cachefile, F_OK) == -1)
    if (download_dayly_list (info, cachefile) == -1)
    {
      free (cachefile);
      return base;
    }
    
  
  if ((fp = fopen (cachefile, "rb")) == NULL)
  {
    free (cachefile);
    return base;
  }
  
  while (!feof (fp))
  {
    if (fgets (fileline, RECOMMENDED_LINE_SIZE, fp) == NULL)
      break;
      
    fileline[RECOMMENDED_LINE_SIZE - 1] = '\0';
    
    if ((p = strchr (fileline, '\n')) != NULL)
      *p = '\0';
      
    for (n = 0; n < supported_filetypes_count; n++)
      if (types_in_common (info->type, index_to_type (n)))
        if (strncmp (
                &fileline[strlen (fileline) - 3],
                  supported_filetypes_list[n]->extension, 3) == 0)
                  strlist_append_string (base, fileline);
                  
      
    
  }


  fclose (fp);  
  free (cachefile);
  return base;
}

int
move_file_data (FILE *fp, FILE *ofp)
{
  char buf[4096];
  int len;
  
  lockf (fileno (ofp), F_LOCK, 0);
  
  fseek (fp, 0, SEEK_SET);
  fseek (ofp, 0, SEEK_SET);
  
  while (!feof (fp))
  {
    if ((len = fread (buf, 1, 4096, fp)) > 0)
      if (fwrite (buf, 1, len, ofp) < len)
        return -1;
  }
  
  fseek (fp, 0, SEEK_SET);
  fseek (ofp, 0, SEEK_SET);
  
  lockf (fileno (ofp), F_ULOCK, 0);
  
  return 0;
}

FILE *
cache_get_ionogram (struct ionogram_filename *info)
{
  char *filename;
  char *cache_path;
  char *given_base_url;
  char *url;
  FILE *fp, *dest, *tmp;
  
  int i, n;
  
  
  if ((filename = ionogram_filename_str (info)) == NULL)
    return NULL;
  
  cache_path = strbuild ("%s/%s", 
    get_ionowatch_cache_dir (),
    filename);
    
  if ((fp = fopen (cache_path, "rb")) != NULL)
  {
    free (cache_path);
    free (filename);
    
    return fp;
  }
  
  if ((n = type_to_index (info->type)) == -1)
    return NULL;
  
  if ((fp = tmpfile ()) == NULL)
  {
    ERROR ("couldn't open tmpfile for writing: %s\n", strerror (errno));
    free (filename);
    free (cache_path);
    return NULL;
  }

  for (i = 0; i < inventories_count; i++)
  {
    if (inventories_list[i] == NULL)
      continue;
    
    given_base_url = (supported_filetypes_list[n]->build_url_callback) 
            (info, inventories_list[i]->url);
        
    
    url = strbuild ("%s/%s", given_base_url, filename);
    
    /* We need to update file pointer because the failure could have happened
       in a half of transaction */
         
    fseek (fp, 0, SEEK_SET);
    
    if (http_download (url, fp) != -1)
    {
      free (url);
      free (given_base_url);
      free (filename);
      
      if ((dest = fopen (cache_path, "w+b")) == NULL)
      {
        ERROR ("couldn't open %s for writing: %s\n", cache_path, strerror (errno));
        
        fclose (fp);
        free (cache_path);
        return NULL;
      }
      
      move_file_data (fp, dest);

      fclose (fp);
      free (cache_path);
            
      return dest;
    }
    
    free (url);
  }
  
  free (given_base_url);
  fclose (fp);
  unlink (cache_path);
 
  free (filename);
  free (cache_path);
      
  return NULL;    
}

void
register_ionogram_inventory (const char *name, const char *url)
{
  struct ionogram_index *index;
  
  index = ionogram_index_new (name, url);
  
  PTR_LIST_APPEND (inventories, index);
}



