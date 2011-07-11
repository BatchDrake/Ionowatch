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

#include "gui.h"

PTR_LIST (struct message_handler, msghandler);

void
ionowatch_register_message_handler (int msgid, message_handler_func func)
{
  struct message_handler *msg;
  
  msg = g_malloc (sizeof (struct message_handler));
  
  msg->msgid = msgid;
  msg->callback = func;
  
  PTR_LIST_APPEND (msghandler, msg);
}

/* Why not call message handler directly ? */

static gboolean
msg_handler_idle_exec (gpointer data)
{
  struct thread_message *msg;
  int i, count;
  
  msg = (struct thread_message *) data;
  
  count = 0;
  
  for (i = 0; i < msghandler_count; i++)
    if (msghandler_list[i] != NULL)
      if (msghandler_list[i]->msgid == msg->msgid)
      {
        (msghandler_list[i]->callback) (msg->msgid, msg->data);
        count++;
      }
  
  if (!count)
    g_printf ("spurious message sent to mainloop: %d\n", msg->msgid);
  
  g_free (data);
  
  return FALSE;
}

void
msg_handler_send (int id, void *data)
{
  struct thread_message *msg;
  
  msg = g_malloc (sizeof (struct thread_message));
  
  msg->msgid = id;
  msg->data = data;
  
  PROTECT (g_idle_add (msg_handler_idle_exec, msg));
}

void
ionowatch_delayed_handler (int msgid, void *data)
{
  void (*func) (void);
  
  enter_callback_context ();
  
  func = (void (*) (void)) data;
  
  (func) ();
  
  leave_callback_context ();
}

void
ionowatch_exec_delayed (void (*func) (void))
{
  msg_handler_send (MESSAGE_EXEC_DELAYED, func);
}

void
ionowatch_datasource_event_handler (int msgid, void *data)
{
  ionowatch_evaluate_all ();
}

void
ionowatch_init_message_handlers (void)
{
  ionowatch_register_message_handler (MESSAGE_EXEC_DELAYED, ionowatch_delayed_handler);
  ionowatch_register_message_handler (MESSAGE_DATASOURCE_EVENT, ionowatch_datasource_event_handler);
}

