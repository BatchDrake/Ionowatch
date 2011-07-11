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

#ifndef _GUI_MESSAGE_H
#define _GUI_MESSAGE_H

#define MESSAGE_EXEC_DELAYED     0
#define MESSAGE_DATASOURCE_EVENT 1

#define DATASOURCE_EVENT_NEW_DATA ((void *) 0)

struct thread_message
{
  int msgid;
  void *data;
};

typedef void (*message_handler_func) (int, void *);

struct message_handler
{
  message_handler_func callback;
  int msgid;
};


void ionowatch_init_message_handlers (void);
void ionowatch_exec_delayed (void (*) (void));
void msg_handler_send (int, void *);

#endif /* _GUI_MESSAGE_H */

