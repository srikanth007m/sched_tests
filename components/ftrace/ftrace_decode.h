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

/* ftrace_decode - framework to decode ftrace data */

#ifndef _FTRACE_DECODE_H
#define _FTRACE_DECODE_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*ftrace_decode_callback_fn)(void * data,int cpu, double time, long long * number_value, const char ** str_value); 
extern bool ftrace_decode_register(const char * event, ftrace_decode_callback_fn fn, void * data, const char ** number, const char ** str);
extern bool ftrace_decode_unregister(const char * event, ftrace_decode_callback_fn fn);
extern void ftrace_decode_process(const char * file);
extern bool ftrace_decode_save(const char * file);
extern void ftrace_decode_init(void);
extern void ftrace_decode_cleanup(void);
#ifdef __cplusplus
}
#endif
#endif
