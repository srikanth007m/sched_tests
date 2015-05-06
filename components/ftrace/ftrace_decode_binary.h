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

/* ftrace_decode_binary - framework to decode binary ftrace data */

#ifndef _FTRACE_DECODE_BINARY
#define _FTRACE_DECODE_BINARY

#ifdef __cplusplus
extern "C" {
#endif
#include "ftrace_decode.h"
#ifdef __cplusplus
}
#endif

#include <stdio.h>

/* this object has the offset/size of fields of one event */
class ftrace_decode_binary_event
{
public:
	const char * debug_prefix(void);
	static const int field_max = 16;
	int field_type;

	ftrace_decode_binary_event(const char * init_name);
	~ftrace_decode_binary_event();
	int id;
	int add_field(const char * field_name);
	bool read_string(const char * data, char * string, int string_size, int field_nb) const;
	unsigned long long read_number_unsigned(const char * data, int field_nb) const;
	long long read_number_signed(const char * data, int field_nb) const;
	bool is_number_signed(int field_nb) const;
	int get_offset(int field_nb) const;
	int get_size(int field_nb) const;
	const char * name;
private:
	int used;
	struct
	{
		int offset;
		int size;
		int is_signed;
	} fields[field_max];
	struct default_t
	{
		const char * name;
		const char * field_name;
		int offset;
		int size;
		int is_signed;
		int id;
	};
	char * filename;
};

/* this object allow to get the RAW event from the data read from trace_pipe_raw */
class ftrace_decode_binary_header
{
public:
	ftrace_decode_binary_header();
	void set_buffer(const char * buffer, int size);
	int get_length() const;
	bool end_of_buffer() const;
	unsigned long long get_time() const;
	const char * get_current_event() const;
	void get_next_event();
private:
	/* header_page must be defined before field_*
	 * because field_* use header_page it in initialization list */
	ftrace_decode_binary_event header_page; 
	static const int header_compressed_size = 4;
	const int field_timestamp;
	const int field_commit;
	const int field_data;

	const char * data;
	unsigned long long time;
	int length;
	bool eob;
	long long commit;
	long long size_remaining;
	long long size_data_remaining;
};


/* this object allow to read trace data dispatch them */
class ftrace_decode_binary_dispatch
{
public:
	ftrace_decode_binary_dispatch();
	virtual	~ftrace_decode_binary_dispatch();
	bool register_callback(const char * event, ftrace_decode_callback_fn fn, void * data, const char ** number, const char ** str);
	bool unregister(const char * event, ftrace_decode_callback_fn fn);
	int ftrace_decode_get_index(const char * name);
	long long ftrace_decode_get_value(int index);
	const char * ftrace_decode_get_str(int index);
protected:
	static const int page_size = 4096;
	static const int cpu_max = 8;
	static const int callback_max = 32;
	struct callback_args
	{
		ftrace_decode_binary_event * event;
		ftrace_decode_callback_fn fn;
		void * data;
		int number_nb;
		int str_nb;
		int * number_index;
		int * str_index;
		long long * number_value;
		const char ** str_value;
	};
	virtual int cpu_buffer_read(char * buffer, int size, int cpu) = 0;
	void process_data(void);

	const char * current_event_data;
	int current_callback;

private:
	int callback_used;
	callback_args callbacks[callback_max];
	int callbacks_id[callback_max];

	static const int temp_str_size = 512;
	char temp_str[temp_str_size];
};

class ftrace_decode_binary_dispatch_file : public ftrace_decode_binary_dispatch
{
public:
	bool save(const char * file);
	bool process(const char * file);
protected:
	virtual int cpu_buffer_read(char * buffer, int size, int cpu);
private:
	int cpu_nb;
	int cpu_size_current[cpu_max];
	int cpu_size[cpu_max];
	FILE * fd;
};
#endif
