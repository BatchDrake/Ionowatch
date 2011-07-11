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

#ifndef _GUI_TREE_H
#define _GUI_TREE_H

#define FILE_TREE_NODE_TYPE_ROOT      -1
#define FILE_TREE_NODE_TYPE_YEAR       0
#define FILE_TREE_NODE_TYPE_DAY        1
#define FILE_TREE_NODE_TYPE_TYPE       2
#define FILE_TREE_NODE_TYPE_FILE       3

struct file_tree_node
{
  int type;
  int updated;
  int state;
  
  char *name;
  char *url;
  
  int   tree_request;
  int   owned_by_thread;
  struct station_info *station;
  
  GtkTreeIter iter; /* Related to repo_tree_store */
  
  PTR_LIST (struct file_tree_node, child);
};

void ionowatch_queue_file_list (struct file_tree_node *);
struct file_tree_node *ionowatch_get_fake_root (void);

#endif /* _GUI_TREE_H */

