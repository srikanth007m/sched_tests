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

/* ftrace_dl - framework to call library that will analyze ftrace */

#ifndef _FTRACE_DL_API_H
#define _FTRACE_DL_API_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
extern void * ftrace_dl_load(const char * name);
extern bool ftrace_dl_unload(void * handle);

/* on success the caller must free the returned pointer */
extern char * ftrace_dl_getinfo(const char * name);

struct ftrace_dl_enum;
extern struct ftrace_dl_enum * ftrace_dl_enum_open(void);
extern bool ftrace_dl_enum_next(struct ftrace_dl_enum * handle, const char ** libname, const char ** info);
extern void ftrace_dl_enum_close(struct ftrace_dl_enum * handle);

#ifdef __cplusplus
}
#endif

#endif
