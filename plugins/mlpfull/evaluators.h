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


#ifndef _EVALUATORS_H
#define _EVALUATORS_H

int real_height_eval (int, int, int, float, float, float, time_t, float *);
int virtual_height_eval (int, int, int, int, float, float, float, float, time_t, float *);
int eval_plasma_peak_height (int, int, float, float, float, time_t, float *);
int eval_plasma_freq (int, int, float, float, float, float, time_t, float *);
int eval_muf (int, int, float, float, float, time_t, float *);
int eval_tec (int, int, float, float, float, time_t, float *);

#endif /* _EVALUATORS_H */
