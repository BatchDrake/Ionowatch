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
#include <pthread.h>

#include "gui.h"

static GtkDialog *loading_dialog;
static void *(*loading_dialog_func) (void *);
static GThread *loading_dialog_thread;
static int loading_dialog_timeout_flag;

static gboolean
loading_dialog_visible_enough (gpointer unused)
{
  loading_dialog_timeout_flag = 1;
  
  return FALSE;
}

int
ionowatch_loading_dialog_should_be_visible (void)
{
  return !loading_dialog_timeout_flag;
}

/* NOTE: never use PROTECT calls inside callbacks. They run inside the
   GDK lock, so a PROTECT call will end in a deadlock. */
void
IonowatchLoadingDialog_show_cb (GtkWidget *widget, gpointer data)
{
  GError *err;
  
  enter_callback_context ();
  
  loading_dialog_timeout_flag = 0;
  
  g_timeout_add_seconds (
    IONOWATCH_SPLASH_TIMEOUT, loading_dialog_visible_enough, NULL);
  
  if (loading_dialog_func != NULL)
  {
    loading_dialog_thread = 
      g_thread_create ((GThreadFunc) loading_dialog_func, NULL, TRUE, &err);
      
    if (loading_dialog_thread == NULL)
    {
      ERROR ("internal error creating thread: %s\n", err->message);
      g_error_free (err);
      exit (1);
    }
  }
  
  leave_callback_context ();
}


void
ionowatch_loading_dialog_set_delayed_func (void *(*func) (void *))
{
  loading_dialog_func = func;
}

void
ionowatch_loading_dialog_set_text (const char *text)
{
  GtkLabel *label;

  if (loading_dialog == NULL)
  {
    ERROR ("dialog not shown, can't set text! (bug)\n");
    return;
  }
    
    
  label = GTK_LABEL (ionowatch_fetch_widget ("IonowatchLoadingDialogLoadingLabel"));
  
  PROTECT (gtk_label_set_text (label, text));
}

void
ionowatch_loading_dialog_show (const char *text)
{
  if (loading_dialog != NULL)
  {
    ERROR ("dialog already shown, bad call performed! (bug)\n");
    return;
  }
  
  loading_dialog = GTK_DIALOG (ionowatch_fetch_widget ("IonowatchLoadingDialog"));
  
  ionowatch_loading_dialog_set_text (text);
  
  PROTECT (gtk_widget_show_all (GTK_WIDGET (loading_dialog)));
}

void
ionowatch_loading_dialog_close (void)
{
  if (loading_dialog == NULL)
  {
    ERROR ("dialog not shown, can't destroy! (bug)\n");
    return;
  }
  
  PROTECT (gtk_widget_destroy (GTK_WIDGET (loading_dialog)));
  
  loading_dialog = NULL;
}

