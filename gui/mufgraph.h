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

#ifndef _GUI_MUFGRAPH_H
#define _GUI_MUFGRAPH_H

#define MUFGRAPH_FONT_SIZE   12
#define MUFGRAPH_SAMPLES     48.0
#define MUFGRAPH_FONT_FAMILY "Courier"

#define MUFGRAPH_GRID_FREQ_STEPS 10
#define MUFGRAPH_GRID_TIME_STEPS 24

#define MUFGRAPH_FREQ_MAX        100.0

void mufgraph_invalidate (void);
void muf_paint_on_cairo (cairo_t *, int, int);

#endif /* _GUI_MUFGRAPH_H */

