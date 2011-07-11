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

#ifndef _GUI_GLGLOBE_H
#define _GUI_GLGLOBE_H

int ionowatch_gl_globe_init (void);
void gl_globe_notify_repaint (void);
void gl_globe_set_draw_grid (gboolean);
void gl_globe_set_draw_station (gboolean);
void gl_globe_set_time (time_t);
void gl_globe_rotate_to (float, float);
void gl_globe_set_shade (gboolean);
void gl_printf (float, float, char *,...);
void draw_circle (float, float, float, float);

#endif /* _GUI_GLGLOBE_H */

