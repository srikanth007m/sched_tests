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

/* ftrace_decode_ascii - framework to decode ascii ftrace data */

#ifndef _FTRACE_DECODE_ASCII
#define _FTRACE_DECODE_ASCII
#ifdef __cplusplus
extern "C" {
#endif
#include "ftrace_decode.h"
#ifdef __cplusplus
}
#endif


class ftrace_decode_ascii
{
public:
	ftrace_decode_ascii();
	~ftrace_decode_ascii();
	bool register_callback(const char * event, ftrace_decode_callback_fn fn, void * data, const char ** number, const char ** str);
	bool unregister(const char * event, ftrace_decode_callback_fn fn);
	bool process(const char * file);
private:
	static const int param_max = 16;
	static const int events_max = 64;

	struct callback_struct
	{
		ftrace_decode_callback_fn fn;
		void * data;
		const char ** number;
		const char ** str;
		int * number_index;
		int * str_index;
		long long * number_value;
		const char ** str_value;
		struct callback_struct * next;
	};

	struct event_struct
	{
		struct callback_struct *callback;
		char * event_name;
	} events[events_max];

	struct
	{
		int cpu;
		double time;
		int param_nb;
		const char * event_name;
		struct
		{
			long long value;
			const char * str; /* if NULL, value is definded */
			const char * name;
		} params[param_max];
	} event_current;

	char ftrace_decode_line[1048576];
	char * skip_space(char * line);
	char * get_event(char * line);
	char * get_param(char * line);
	bool get_timecpu(void);
	bool callback_call(void);
	int get_index(const char * name);
	long long get_value(int index);
	const char * get_str(int index);
};
#endif
