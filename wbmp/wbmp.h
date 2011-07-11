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

#ifndef _WBMP_H
#define _WBMP_H

#include "layout.h"

struct draw
{
  int width;
  int height;
  int total_size;
  
  BYTE *pixels;
};

struct draw *draw_new (int, int);
void draw_free (struct draw *);
struct draw *draw_from_bmp (const char *);
int draw_to_bmp (const char *, struct draw *);

void  draw_pset (struct draw *, int, int, DWORD);
DWORD draw_pget (struct draw *, int, int);

#endif /* _WBMP_H */

