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
#include <pwd.h>
#include <unistd.h>

#include <ionowatch.h>

static char *ionowatch_config_dir;
static char *cache_dir;

char *
get_ionowatch_config_dir (void)
{
  struct passwd *pw;
  
  if (ionowatch_config_dir == NULL)
  {
    if ((pw = getpwuid (geteuid ())) == NULL)
    {
      ERROR ("foreign uid in system!\n");
      return NULL;
    }

    ionowatch_config_dir = strbuild ("%s/.ionowatch", pw->pw_dir);

    if (access (ionowatch_config_dir, F_OK) == -1)
    {
      if (mkdir (ionowatch_config_dir, 0700) == -1)
      {
        ERROR ("couldn't create %s: %s\n", ionowatch_config_dir, strerror (errno));
        free (ionowatch_config_dir);
        return NULL;
      }
    }
  }
  
  return ionowatch_config_dir;
}

char *
get_ionowatch_cache_dir (void)
{
  if (cache_dir == NULL)
  { 
    cache_dir = strbuild ("%s/cache", get_ionowatch_config_dir ());
    
    if (access (cache_dir, F_OK) == -1)
    {
      if (mkdir (cache_dir, 0700) == -1)
      {
        ERROR ("couldn't create %s: %s\n", cache_dir, strerror (errno));
        free (cache_dir);
        return NULL;
      }
    }
  }
  
  return cache_dir;
}

char *
locate_config_file (const char *name, int flags)
{
  char *path;
  int access_flags;
  
  access_flags = F_OK;
  
  if (flags & CONFIG_READ)
    access_flags |= R_OK;
  else if (flags & CONFIG_WRITE)
    access_flags |= W_OK;
      
  if (flags & CONFIG_LOCAL)
  {
    path = strbuild ("%s/%s", 
      get_ionowatch_config_dir (),
      name);
      
    if (access (path, access_flags) != -1)
      return path;
      
    if (flags & CONFIG_CREAT)
    {
      if (access (get_ionowatch_config_dir (), W_OK) != -1)
        return path;
    }
    
    free (path);
  }
  
  if (flags & CONFIG_GLOBAL)
  {
    path = strbuild (IONOWATCH_GLOBAL_DIR "/%s", 
      name);
      
    if (access (path, access_flags) != -1)
      return path;
    
    if (flags & CONFIG_CREAT)
    {
      if (access (IONOWATCH_GLOBAL_DIR, W_OK) != -1)
        return path;
    }
      
    free (path);  
  }
  
  if (name[0] == '/')
    if (access (name, flags) != -1)
      return xstrdup (name);
      
  return NULL;
}


