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


#ifndef _SUNSPOT_H
#define _SUNSPOT_H

struct sunspot_record
{
  int year, month;
  float number;
};

void sunspot_add_record (int, int, float);
float get_monthly_sunspot_number (time_t);
int init_sunspot_cache (void);
void destroy_sunspot_cache (void);
int update_sunspot_cache (void);

#endif /* _SUNSPOT_H */

