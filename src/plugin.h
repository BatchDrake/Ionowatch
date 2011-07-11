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


#ifndef _PLUGIN_H
#define _PLUGIN_H

#define PLUGIN_MAGIC "I0.1"

#define DECLARE_PLUGIN(name) \
  char plugin_magic[] = PLUGIN_MAGIC; \
  char plugin_name[] = name; \
  
struct ionowatch_plugin
{
  void *handle;
  char *name;
  char *path;
  
  void (*entry) (void);
};

int load_all_plugins (void);
void init_all_plugins (void);

#endif /* _PLUGIN_H */

