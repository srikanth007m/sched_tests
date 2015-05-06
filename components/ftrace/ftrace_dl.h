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

#ifndef _FTRACE_DL_H
#define _FTRACE_DL_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef FTRACE_DL_SO
/* In ftrace_dl.c */
struct ftrace_dl_libs_chained_struct
{
	const char * filename;
	void * (*extern_ftrace_dl_init)(void);
	const char * (*extern_ftrace_dl_info)(void);
	bool (*extern_ftrace_dl_cleanup)(void * data);
	struct ftrace_dl_libs_chained_struct * next;
};
extern struct ftrace_dl_libs_chained_struct *
	ftrace_dl_libs_chained;

/* for current ftrace_so_*.c */
#ifndef _FTRACE_DL_C
/* shoud be provided in the .so */
static void * extern_ftrace_dl_init(void);
static const char * extern_ftrace_dl_info(void);
/* return value tell if the trace follow the library rules:
 * for instance migration times not too big with libmigration */
static bool extern_ftrace_dl_cleanup(void * data);

#ifndef FTRACE_SO_LIBNAME
#error "FTRACE_SO_LIBNAME must be defined to the library name of this"
#endif
static struct ftrace_dl_libs_chained_struct ftrace_dl_libs;

/* does the chaining before main() */
static  __attribute__((constructor)) void ftrace_dl_preinit(void)
{
	ftrace_dl_libs.filename = FTRACE_SO_LIBNAME ;
	ftrace_dl_libs.extern_ftrace_dl_init = extern_ftrace_dl_init,
	ftrace_dl_libs.extern_ftrace_dl_info = extern_ftrace_dl_info,
	ftrace_dl_libs.extern_ftrace_dl_cleanup = extern_ftrace_dl_cleanup,
	ftrace_dl_libs.next = ftrace_dl_libs_chained;
	ftrace_dl_libs_chained = &ftrace_dl_libs;
}
#define FTRACE_SO_INIT(f) \
	static void * extern_ftrace_dl_init(void)\
	{\
		return f();\
	}
#define FTRACE_SO_CLEANUP(f) \
	static bool extern_ftrace_dl_cleanup(void * data)\
	{\
		return f(data);\
	}
#define FTRACE_SO_INFO(f) \
	static const char* extern_ftrace_dl_info(void)\
	{\
		return f();\
	}
#endif /* FTRACE_DL_C */
#endif /* FTRACE_DL_SO */


#ifdef FTRACE_DL_SO
#ifndef _FTRACE_DL_C
/* shoud be provided in the .so */
extern void * extern_ftrace_dl_init(void);
extern const char * extern_ftrace_dl_info(void);
/* return value tell if the trace follow the library rules:
 * for instance migration times not too big with libmigration */
extern bool extern_ftrace_dl_cleanup(void * data);
#define FTRACE_SO_INIT(f) \
	void * extern_ftrace_dl_init(void)\
	{\
		return f();\
	}
#define FTRACE_SO_CLEANUP(f) \
	bool extern_ftrace_dl_cleanup(void * data)\
	{\
		return f(data);\
	}
#define FTRACE_SO_INFO(f) \
	const char* extern_ftrace_dl_info(void)\
	{\
		return f();\
	}
#endif /* _FTRACE_DL_C */
#endif /* FTRACE_DL_SO */

#ifdef __cplusplus
}
#endif

#endif
