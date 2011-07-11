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

#ifndef _GUI_H
#define _GUI_H

#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdk.h>

#include "glglobe.h"
#include "gui.h"
#include "ionograph.h"
#include "message.h"
#include "mufgraph.h"
#include "painter.h"
#include "tree.h"
#include "trig.h"

#define IONOWATCH_SPLASH_TIMEOUT 3
#define RECENT_MAX_SAVE          100

/* Things that I learn from the Linux kernel source code: wrapping blocks of
   code inside do {...} while (0) */

#define PROTECT(call)         \
  do                          \
  {                           \
    if (must_lock ())         \
      gdk_threads_enter ();   \
    call;                     \
    if (must_lock ())         \
      gdk_threads_leave ();   \
  }                           \
  while (0)

#ifdef ERROR
# undef ERROR
# define ERROR(fmt, arg...) ionowatch_set_last_error ("Last error at %s: " fmt, get_curr_ctime (), ##arg)
#endif

#ifdef WARNING
# undef WARNING
# define WARNING(fmt, arg...) ionowatch_set_last_error ("Last warning at %s: " fmt, get_curr_ctime (), ##arg)
#endif

#ifdef NOTICE
# undef NOTICE
# define NOTICE(fmt, arg...) ionowatch_set_last_error ("Last notice at %s: " fmt, get_curr_ctime (), ##arg)
#endif





GtkWidget *ionowatch_fetch_widget (const char *);
GObject *ionowatch_fetch_object (const char *);

int ionowatch_gtk_early_init (int *, char **[]);
void ionowatch_main_loop (void);

void ionowatch_loading_dialog_set_text (const char *);
void ionowatch_loading_dialog_show (const char *);
void ionowatch_loading_dialog_close (void);
int ionowatch_loading_dialog_should_be_visible (void);
void ionowatch_fatal_error (const char *message);
void ionowatch_dispatch_pending_events (void);


int must_lock (void);
int enter_callback_context (void);
int leave_callback_context (void);

void ionowatch_refresh_models (void);
void ionowatch_init_models (void);

void ionowatch_set_actual_data (void);



void ionowatch_switch_datasource (struct datasource *);
void ionowatch_switch_painter (struct painter *);
void ionowatch_refresh_current_painter (void);


void ionowatch_generic_error (const char *, const char *);
void ionowatch_select_ionogram (struct station_info *, time_t);
void ionowatch_parse_ionogram_file (const char *);
void init_default_painter (void);
void ionowatch_select_station (struct station_info *);
void ionowatch_notify_datasource_list_event (void);
int ionowatch_refresh_predictor_select_store (void);
void ionowatch_save_recent_file_list (void);
void ionowatch_load_recent_file_list (void);
void ionowatch_set_status (const char *, ...);
void ionowatch_set_error (const char *, ...);
void recent_file_activate_cb (GtkWidget *, gpointer);
struct station_info *ionowatch_get_selected_station (void);
void ionowatch_switch_datasource (struct datasource *);
struct datasource *ionowatch_get_selected_predictor (void);
int export_matlab (const char *);



#endif /* _GUI_H */

