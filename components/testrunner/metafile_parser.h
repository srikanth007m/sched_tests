/*
 * Copyright (C) 2015 ARM Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _METAFILE_PARSER_H_
#define _METAFILE_PARSER_H_

int get_description(char **description, char *metafilepath);
int get_passthreshold(float *threashold, char *metafilepath);
int get_uniquename(char** name, char *metafilepath);
int get_isdeprecated(int *deprecated, char *metafilepath);

#endif
