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

#ifndef _GUI_IONOGRAPH_H
#define _GUI_IONOGRAPH_H

#define IONOGRAM_FONT_SIZE   12
#define IONOGRAM_SAMPLES     60
#define IONOGRAM_FONT_FAMILY "Courier"

#define IONOGRAM_FREQ_MAX    10.0
#define IONOGRAM_HEIGHT_MAX  700.0

#define IONOGRAM_GRID_HEIGHT_STEPS 20
#define IONOGRAM_GRID_FREQ_STEPS   20

void ionogram_invalidate (void);
void ionogram_paint_on_cairo (cairo_t *, int, int);

#endif /* _GUI_IONOGRAPH_H */

