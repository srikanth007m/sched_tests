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

extern "C"
{
#include "ftrace_decode.h"
};

#include "ftrace_decode_ascii.h"
#include "ftrace_decode_binary.h"

class ftrace_decode_ascii ascii;
class ftrace_decode_binary_dispatch_file binary;


extern "C" void ftrace_decode_process(const char * file)
{
	if (ascii.process(file))
		return;
	binary.process(file);
}

extern "C" void ftrace_decode_init(void)
{
}

extern "C" bool ftrace_decode_save(const char * file)
{
	return binary.save(file);
}

extern "C" void ftrace_decode_cleanup(void)
{
}

extern "C" bool ftrace_decode_register(const char * event, ftrace_decode_callback_fn fn, void * data, const char ** number, const char ** str)
{
	return ascii.register_callback(event, fn, data, number, str)
		&& binary.register_callback(event, fn, data, number, str);
}

bool ftrace_decode_unregister(const char * event, ftrace_decode_callback_fn fn)
{
	return ascii.unregister(event, fn) && binary.unregister(event, fn);
}


