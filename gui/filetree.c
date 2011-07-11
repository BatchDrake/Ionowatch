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

#include <ionowatch.h>
#include <pthread.h>

#include "gui.h"

struct parentchild
{
  struct file_tree_node *parent;
  struct strlist *strlist;
  struct station_info *station;

  int type;
};

struct file_tree_node *file_tree_root;
pthread_mutex_t file_tree_mutex = PTHREAD_MUTEX_INITIALIZER;

extern struct station_info *selected_station;

extern GtkTreeView *repo_tree_view;
extern GtkTreeStore *repo_tree_store;

static tree_request_count;

struct file_tree_node *
file_tree_node_new (const char *name, const char *url)
{
  struct file_tree_node *new;
  
  new = xmalloc (sizeof (struct file_tree_node));
  
  memset (new, 0, sizeof (struct file_tree_node));
  
  if (name != NULL)
    new->name = xstrdup (name);
    
  if (url != NULL)
    new->url = xstrdup (url);
  
  new->station = selected_station;
  
  return new;
}

void
file_tree_node_destroy_recursive (struct file_tree_node *node)
{
  int i;
  
  if (node->owned_by_thread) /* Can't delete, node is in use */
    return;
    
  for (i = 0; i < node->child_count; i++)
    if (node->child_list[i] != NULL)
      file_tree_node_destroy_recursive (node->child_list[i]);
      
  free (node->name);
  free (node->url);
  
  if (node->child_count)
    free (node->child_list);
    
  free (node);
}

void
unsafe_ionowatch_file_tree_refresh_node (struct file_tree_node *node)
{
  gboolean state;
  struct ionogram_filename fn;
  
  char *date;
  char datebuf[30];
  
  if (node->type == FILE_TREE_NODE_TYPE_ROOT)
    return;
    
  state = node->state ? TRUE : FALSE;
  
  date = "";
  
  if (node->type == FILE_TREE_NODE_TYPE_FILE)
    if (ionogram_parse_filename (node->name, &fn) != -1)
    {
      ctime_r (&fn.time, datebuf);
      datebuf[strlen (datebuf) - 1] = 0;
      date = datebuf;
    }
    
  PROTECT (gtk_tree_store_set (repo_tree_store, 
    &node->iter, 0, node->name, 1, date, 2, node->url, 3, state, 5, node, -1));
}

void
ionowatch_file_tree_refresh_node (struct file_tree_node *node)
{
  pthread_mutex_lock (&file_tree_mutex);
  
  unsafe_ionowatch_file_tree_refresh_node (node);
  
  pthread_mutex_unlock (&file_tree_mutex);
}

struct file_tree_node *
unsafe_ionowatch_get_fake_root (void)
{
  if (file_tree_root == NULL)
  {
    file_tree_root = file_tree_node_new (NULL, NULL);
    file_tree_root->type = FILE_TREE_NODE_TYPE_ROOT;
    file_tree_root->tree_request = tree_request_count++;
  }
  
  return file_tree_root;
}

struct file_tree_node *
ionowatch_get_fake_root (void)
{
  struct file_tree_node *result;
  
  pthread_mutex_lock (&file_tree_mutex);
  
  result = unsafe_ionowatch_get_fake_root ();
  
  pthread_mutex_unlock (&file_tree_mutex);
  
  return result;
}



struct parentchild *
parentchild_new (struct file_tree_node *parent, struct strlist *list, struct station_info *station, int type)
{
  struct parentchild *new;
  
  new = xmalloc (sizeof (struct parentchild));
  
  new->parent = parent;
  new->strlist = list;
  new->station = station;
  new->type = type;
  
  return new;
}

void
parentchild_destroy (struct parentchild *pc)
{
  strlist_destroy (pc->strlist);
  free (pc);
}


/* TODO: learn more about big GTK+ lock */
void
unsafe_ionowatch_file_tree_append (struct file_tree_node *parent, 
                                   struct file_tree_node *child)
{   
  PTR_LIST_APPEND (parent->child, child);
  
  child->tree_request = parent->tree_request;
  
  if (parent->type == FILE_TREE_NODE_TYPE_ROOT)
    PROTECT (gtk_tree_store_append (repo_tree_store, &child->iter, NULL));
  else
    PROTECT (gtk_tree_store_append (repo_tree_store, &child->iter, &parent->iter));

  
  unsafe_ionowatch_file_tree_refresh_node (child);
}

void
ionowatch_file_tree_append (struct file_tree_node *parent, 
                            struct file_tree_node *child)
{
  pthread_mutex_lock (&file_tree_mutex);
  
  unsafe_ionowatch_file_tree_append (parent, child);
  
  pthread_mutex_unlock (&file_tree_mutex);
}

/* TODO: lock station changes */
/* TODO: build a list of stations being retrieved. Checking if the station changed is just not enough */
/* XXX: this fails when downloading the same station filelist twice and one of the parents were destroyed. Fix. */

gboolean
file_tree_add_nodes_idle (gpointer data)
{
  GtkTreePath *path;
  
  struct parentchild *pc;
  struct file_tree_node *node;
  char *url;
  int i;
  
  pc = (struct parentchild *) data;
  
  pthread_mutex_lock (&file_tree_mutex);
  
  /* Check if this node is of the same request */
  if (pc->parent->tree_request == file_tree_root->tree_request)
  {
    if (pc->parent->type == FILE_TREE_NODE_TYPE_ROOT)
      PROTECT (gtk_tree_store_clear (repo_tree_store)); /* Clear the "fetching files" row */
        
    for (i = 0; i < pc->strlist->strings_count; i++)
      if (pc->strlist->strings_list[i] != NULL)
      {
        url = strbuild ("%s/%s/", pc->parent->url, pc->strlist->strings_list[i]);
          
        node = file_tree_node_new (pc->strlist->strings_list[i], url);
        
        free (url);
        
        node->type = pc->type;
        
        unsafe_ionowatch_file_tree_append (pc->parent, node);                 
      }
      
      
    pc->parent->state = 0;
    pc->parent->updated = 1;
    
    unsafe_ionowatch_file_tree_refresh_node (pc->parent);
    
    if (pc->parent->type != FILE_TREE_NODE_TYPE_ROOT)
    {
      PROTECT (path = gtk_tree_model_get_path (GTK_TREE_MODEL (repo_tree_store), &pc->parent->iter));
      PROTECT (gtk_tree_view_expand_row (repo_tree_view, path, TRUE));
      PROTECT (gtk_tree_path_free (path));
    }
  }
  else
  {
    pc->parent->owned_by_thread = 0;
    file_tree_node_destroy_recursive (pc->parent); /* Too late, discarding it */
    
    NOTICE ("tree request doesn't fit (%d != %d)\n", pc->parent->tree_request, file_tree_root->tree_request);
    
  }
  
  ionowatch_set_status ("Request done");
  
  pc->parent->owned_by_thread = 0;
  
  parentchild_destroy (pc);
  
  pthread_mutex_unlock (&file_tree_mutex);
  
  return FALSE;
}

static void *
file_list_fetcher_thread (void *arg)
{
  struct file_tree_node *node;
  struct strlist *strlist;
  struct station_info *curr;
  
  char *url;
  
  ionowatch_set_status ("Background: requesting remote file list...");
  
  pthread_mutex_lock (&file_tree_mutex);
  
  if ((node = (struct file_tree_node *) arg) == NULL)
    node = file_tree_root;
  
  if (node->owned_by_thread)
  {
    WARNING ("trying to download data from a node already owned");
    pthread_mutex_unlock (&file_tree_mutex);
    return NULL;
  }
  
  curr = node->station;
  
  node->owned_by_thread = 1; /* Mark this one as used by thread*/ 
    
  pthread_mutex_unlock (&file_tree_mutex);
  
  
  strlist = parse_apache_index (node->url);

  pthread_mutex_lock (&file_tree_mutex);
  
  /* TODO: detect what we're downloading */
  PROTECT (g_idle_add (file_tree_add_nodes_idle, parentchild_new (node, strlist, curr, node->type + 1)));
  pthread_mutex_unlock (&file_tree_mutex);
  
  return NULL;
}

/* All threads gathering information in the background about an station we've
   just changed from must be killed before performing any change in the
   file tree. One reason more to have a list of threads */
void
ionowatch_queue_file_list (struct file_tree_node *node)
{
  /* XXX: this may cause leaks, get thread structs out of here */
  pthread_t thread;
  
  if (node->type != FILE_TREE_NODE_TYPE_ROOT)
  {
    node->state = 1; /* Raaace condition */
    ionowatch_file_tree_refresh_node (node);
  }
  
  pthread_create (&thread, NULL, file_list_fetcher_thread, node); /* XXX: Pass information about current station OUT the node structure */
}

void
unsafe_ionowatch_clear_file_tree (void)
{
  if (file_tree_root != NULL)
  {
    file_tree_node_destroy_recursive (file_tree_root);
    file_tree_root = NULL;
  }
   
  PROTECT (gtk_tree_store_clear (repo_tree_store));
}

void
ionowatch_clear_file_tree (void)
{
  pthread_mutex_lock (&file_tree_mutex);
  
  unsafe_ionowatch_clear_file_tree ();
  
  pthread_mutex_unlock (&file_tree_mutex);
}

/* TODO: this is not strictly the right behaviour, but by the time being it
   will more than enough to give some feedback to the user */
void
unsafe_ionowatch_set_fetching_filelist (void)
{
  GtkTreeIter iter;
  
  PROTECT (gtk_tree_store_append (repo_tree_store, &iter, NULL));
  PROTECT (gtk_tree_store_set (repo_tree_store, &iter, 0, "Fetching...", 3, TRUE, -1));
}

void
ionowatch_set_fetching_filelist (void)
{
  pthread_mutex_lock (&file_tree_mutex);
  
  unsafe_ionowatch_set_fetching_filelist ();
  
  pthread_mutex_unlock (&file_tree_mutex);
}


