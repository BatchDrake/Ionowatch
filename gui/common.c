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

#include <ionowatch.h>
#include <locale.h>

#include "gui.h"

static GtkBuilder *builder;
static __thread  int callback_context_depth;

int
enter_callback_context (void)
{
  callback_context_depth++;
}

int
leave_callback_context (void)
{
  if (--callback_context_depth < 0)
  {
    ERROR ("leave_callback_context without enter\n");
    abort ();
  }
}

int
must_lock (void)
{
  return !callback_context_depth;
}

void
ionowatch_dispatch_pending_events (void)
{
  while (g_main_iteration (FALSE));
}

void
ionowatch_generic_error (const char *message, const char *title)
{
  GtkWidget *widget;
  
  PROTECT (widget = gtk_message_dialog_new 
    (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
      "%s", message));
      
  gtk_window_set_title (GTK_WINDOW (widget), title);
  PROTECT (gtk_dialog_run (GTK_DIALOG (widget)));
  
  PROTECT (gtk_widget_destroy (widget));
}


void
ionowatch_fatal_error (const char *message)
{
  ionowatch_generic_error (message, "Fatal error");
}


GtkWidget *
ionowatch_fetch_widget (const char *name)
{
  GtkWidget *result;
  
  PROTECT (result = GTK_WIDGET (gtk_builder_get_object (builder, name)));
  
  return result;
}


GObject *
ionowatch_fetch_object (const char *name)
{
  GObject *result;
  
  PROTECT (result = G_OBJECT (gtk_builder_get_object (builder, name)));
  
  return result;
}


void
ionowatch_display_main_window ()
{
  GtkWidget *main;
  
  main = ionowatch_fetch_widget ("IonowatchMainWindow");
  
  PROTECT (gtk_widget_show_all (main));
}

/* TODO: move every single GTK+ function to mainloop via messages, this
         should run inside a thread. */
void *
ionowatch_init_delayed (void *unused)
{
  ionowatch_loading_dialog_set_text ("Loading stations...");
  
  if (load_stations () == -1)
  {
    ionowatch_fatal_error (
      "Unable to load station list file. This usually happens after a bad "
      "or partial installation. Reinstall ionowatch and try again");
      
    PROTECT (gtk_main_quit ());
  }
  
  ionowatch_loading_dialog_set_text (
    "Updating sunspot cache from the Internet...");
    
  if (init_sunspot_cache () == -1)
  {
    ionowatch_fatal_error (
      "Unable to update sunspot cache. This usually happens when there's no "
      "previous cache and Ionowatch is not able to connect to the Internet "
      "to find a newer one. Verify your connection and try again.");
      
    PROTECT (gtk_main_quit ());
  }
  
  ionowatch_loading_dialog_set_text ("Registering URLs and data sources... ");
  
  register_ionogram_inventory 
    ("Master Ionosonde Data Set", "http://ngdc.noaa.gov/ionosonde/MIDS/data/");
  
  ionogram_filetype_register 
    ("SAO", "Standard scaled data archive", parse_sao_file, sao_build_url);
    
  ionogram_filetype_register 
    ("MMM", "Modified maximum method ionogram", ionogram_from_mmm, blockfile_build_url);
    
  ionogram_filetype_register 
    ("SBF", "Single block format ionogram", ionogram_from_sbf, blockfile_build_url);
    
  ionogram_filetype_register 
    ("RSF", "Routine scientific format ionogram", ionogram_from_rsf, blockfile_build_url);
  
  scaled_data_datasource_init ();
  
  ionowatch_loading_dialog_set_text ("Loading plugins...");
  
  load_all_plugins ();
  
  ionowatch_loading_dialog_set_text ("Load done, setting up GUI...");
 // while (ionowatch_loading_dialog_should_be_visible ());
  
  ionowatch_exec_delayed (ionowatch_init_models);
  ionowatch_exec_delayed (init_all_plugins);
  ionowatch_exec_delayed (ionowatch_display_main_window);
  ionowatch_exec_delayed (ionowatch_notify_datasource_list_event);
  ionowatch_exec_delayed (ionowatch_loading_dialog_close);
}


int
ionowatch_gtk_early_init (int *argc, char **argv[])
{
  char *dir;
  char *path;
  
  g_thread_init (NULL);
  gdk_threads_init ();
    
  glutInit (argc, *argv);
  
  if (gtk_init_check (argc, argv) == FALSE)
    return -1;

  /* This is because GTK insists to be a smartass */
  setlocale (LC_NUMERIC, "C");
  
  if ((dir = get_ionowatch_config_dir ()) == NULL)
  {
    ERROR (
      "Unable to setup config directory. This usually happens when there's "
      "no space left on your home folder or it has wrong permissions. Check "
      "your drive space and home folder permissions and try again.\n");
      
    return -1;
  }
  
  if ((dir = get_ionowatch_cache_dir ()) == NULL)
  {
    ERROR (
      "Unable to setup cache directory. This usually happens when there's "
      "no space left on your home folder. Delete some files and try again.\n");
      
    return -1;
  }
  
  builder = gtk_builder_new ();
  
  if ((path = locate_config_file ("ionowatch.ui", CONFIG_READ | CONFIG_GLOBAL)) == NULL)
  {
    ERROR (
      "Unable to locate ionowatch interface file (ionowatch.ui). This usually "
      "happens after a partial installation.\n");
      
    return -1;
  }
  
  gtk_builder_add_from_file (builder, path, NULL);
  
  free (path);
  
  gtk_builder_connect_signals (builder, NULL);

  ionowatch_loading_dialog_set_delayed_func (ionowatch_init_delayed);

  if (ionowatch_gl_globe_init () == -1)
    return -1;
  
  ionowatch_init_message_handlers ();
  
  return 0;
}

void
ionowatch_main_loop (void)
{
  PROTECT (gtk_main ());
}

