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

#ifndef _FTRACE_DECODE_INTERNAL
#define _FTRACE_DECODE_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif
#include "ftrace_decode.h"
#ifdef __cplusplus
}
#endif

class ftrace_decode_abstract
{
public:
	virtual bool register_callback(const char * event, ftrace_decode_callback_fn fn, void * data, const char ** number, const char ** str) = 0;
	virtual bool unregister(const char * event, ftrace_decode_callback_fn fn) = 0;
	virtual 
	virtual ~ftrace_decode_abstract() {};
};

extern class ftrace_decode_abstract * ftrace_decode_ascii_get();
extern class ftrace_decode_abstract * ftrace_decode_binary_get();
#endif

