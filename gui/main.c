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
#include <stdlib.h>
#include <string.h>

#include <ionowatch.h>

#include "gui.h"

gint
main (gint argc, gchar *argv[])
{
  (void) libsao_init ();
  
  if (ionowatch_gtk_early_init (&argc, &argv) == -1)
  {
    ERROR ("couldn't initialize ionowatch GTK+ interface properly\n");
    exit (1);
  }
  
  ionowatch_loading_dialog_show ("Ionowatch is loading, please wait...");
  
  setenv ("TZ", "UTC", 1);
  
  ionowatch_main_loop ();
  
  return EXIT_SUCCESS;
}

