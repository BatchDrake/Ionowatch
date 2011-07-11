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
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <ionowatch.h>

PTR_LIST (struct ionowatch_plugin, plugin);

struct ionowatch_plugin *
ionowatch_plugin_lookup (const char *name)
{
  int i;
  
  for (i = 0; i < plugin_count; i++)
    if (plugin_list[i] != NULL) /* This will be really necessary */
      if (strcmp (plugin_list[i]->name, name) == 0)
        return plugin_list[i];
      
  return NULL;
}

int
ionowatch_plugin_register (const char *path)
{
  void *handle;
  struct ionowatch_plugin **this_plugin;
  void (*entry) (void);
  char *magic;
  char *name;
  struct ionowatch_plugin *new;
  
  if ((handle = dlopen (path, RTLD_NOW)) == NULL)
  {
    ERROR ("%s\n", dlerror ());
    return -1;
  }
  
  if ((magic = dlsym (handle, "plugin_magic")) == NULL)
  {
    ERROR ("%s: invalid plugin magic (not present)\n", path);
    dlclose (handle);
    return -1;
  }
  
  if (memcmp (magic, PLUGIN_MAGIC, 4) != 0)
  {
    ERROR ("%s: invalid plugin magic\n", path);
    dlclose (handle);
    return -1;
  }
  
  if ((name = dlsym (handle, "plugin_name")) == NULL)
  {
    ERROR ("%s: invalid plugin name (not present)\n", path);
    dlclose (handle);
    return -1;
  }
  
  if ((entry = dlsym (handle, "plugin_init")) == NULL)
  {
    ERROR ("%s: plugin has no entry point\n", path);
    dlclose (handle);
    return -1;
  }
  
  
  if ((new = ionowatch_plugin_lookup (name)) != NULL)
  {
    dlclose (handle);
    WARNING ("%s: already loaded in %s\n", path, new->path);
    return 1;  
  }
  
  new = xmalloc (sizeof (struct ionowatch_plugin));
  
  new->handle = handle;
  new->name   = name;
  new->path   = xstrdup (path);
  new->entry  = entry;
  
  this_plugin = dlsym (handle, "this_plugin");
  
  if (this_plugin != NULL)
    *this_plugin = new;
  
  PTR_LIST_APPEND (plugin, new);
  
  return 0;
}

static int
read_plugin_dir (const char *plugin_dir)
{
  DIR *dir;
  struct dirent *ent;
  char *fullpath;
  
  int count;
  
  count = 0;
  
  if ((dir = opendir (plugin_dir)) != NULL)
  {
    while ((ent = readdir (dir)) != NULL)
    {
      if (strncmp (ent->d_name, "lib", 3) ||
          strncmp (&ent->d_name[strlen (ent->d_name) - 3], ".so", 3))
        continue;
        
      fullpath = strbuild ("%s/%s", plugin_dir, ent->d_name);
      
      if (ionowatch_plugin_register (fullpath) == 0)
        count++;
      
      free (fullpath);
    }
    
    closedir (dir);
  }

  return count;
}

int
load_all_plugins (void)
{
  char *plugin_dir;
  int count = 0;
  
  
  DEBUG ("registering locals\n");
  
  plugin_dir = locate_config_file ("plugins", CONFIG_LOCAL | CONFIG_READ);
  
  if (plugin_dir != NULL)
  {
    count += read_plugin_dir (plugin_dir);
    free (plugin_dir);
  }
  
  DEBUG ("registering globals\n");
  
  plugin_dir = locate_config_file ("plugins", CONFIG_GLOBAL | CONFIG_READ);
  
  if (plugin_dir != NULL)
  {
    count += read_plugin_dir (plugin_dir);
    free (plugin_dir);
  }
  else
    DEBUG ("no global dir\n");
    
  DEBUG ("%d plugins loaded\n", count);
  
  return count;
}

void
init_all_plugins (void)
{
  int i;
  
  DEBUG ("initializing all plugins\n");
  
  for (i = 0; i < plugin_count; i++)
    if (plugin_list[i] != NULL)
      (plugin_list[i]->entry) ();
}


