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

#ifndef _BUSY_LOOP_H
#define _BUSY_LOOP_H
#include <stdbool.h>

struct busy_loop_data;
struct busy_loop_data * busy_loop_create(long long nbloop);
bool busy_loop_run(struct busy_loop_data * data);
void busy_loop_destroy(struct busy_loop_data * data);
#endif
