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


#ifndef _CONFIG_FILES_H
#define _CONFIG_FILES_H

#define IONOWATCH_GLOBAL_DIR "/usr/lib/ionowatch"

#define CONFIG_GLOBAL 64
#define CONFIG_LOCAL  128

#define CONFIG_READ   1
#define CONFIG_WRITE  2
#define CONFIG_CREAT  4

char *get_ionowatch_config_dir (void);
char *get_ionowatch_cache_dir (void);
char *locate_config_file (const char *, int);

#endif /* _CONFIG_FILES_H */


