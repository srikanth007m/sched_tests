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

#include <sys/stat.h>
#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/mount.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "ftrace_decode_binary.h"

const char * ftrace_decode_binary_event::debug_prefix(void)
{
	const char * temp = getenv("TRACE_EVENTS_PATH");
	if (temp != NULL)
		return temp;
	return "/sys/kernel/debug/tracing/events/";
}

ftrace_decode_binary_event::~ftrace_decode_binary_event()
{
	if (name != NULL)
		free((char *)name);
	if (filename != NULL)
		free(filename);
}

ftrace_decode_binary_event::ftrace_decode_binary_event(const char * init_name)
	: field_type(-1), id(-1), name(strdup(init_name)),
	used(0), filename(NULL)
{
	filename= (char *)malloc(strlen(debug_prefix())
			+ 2 * strlen(name) + strlen("/format") + 1);
	if (filename == NULL)
		return;
	if (strcmp(name, "header_page") == 0)
		sprintf(filename, "%sheader_page", debug_prefix());
	else if (strcmp(name, "tracing_mark_write") == 0)
		sprintf(filename, "%sprintk/console/format", debug_prefix());
	else
		sprintf(filename, "%s%s/format", debug_prefix(), name);
	FILE * fd = fopen(filename, "r");
	if (fd == NULL)
	{
		/* for name like sched_switch try to look in sched/sched_switch */
		int size;
		size = sprintf(filename, "%s",
				debug_prefix());
		strcat(filename, name);
		if (strchr(filename + size, '_') == NULL)
		{
			//printf("%s %s not found\n", name, filename);
			free(filename);
			filename = NULL;
			return;
		}
		(strchr(filename + size, '_'))[0]= 0;
		strcat(filename, "/");
		strcat(filename, name);
		strcat(filename, "/format");
		fd = fopen(filename, "r");
		if (fd == NULL)
		{
			//printf("%s %s not found\n", name, filename);
			free(filename);
			filename = NULL;
			return;
		}
	}
	if (strcmp(name, "header_page") == 0)
	{
		fclose(fd);
		return;
	}

	field_type = add_field("common_type");
	while (1)
	{
		while (fscanf(fd, "%*[\n]") > 0)
			;
		char line[256];
		if (fscanf(fd, "%[^\n]", line) < 1)
		{
			//printf("PMF sensor ftrace %s not found\n", name);
			break;
		}
		if (sscanf(line,"ID: %d", &id) == 1)
		{
			break;
		}
	}
	fclose(fd);
}

int ftrace_decode_binary_event::get_size(int field_nb) const
{
	if ((field_nb < 0) || (field_nb >= field_max))
		return -1;
	return fields[field_nb].size;
}

int ftrace_decode_binary_event::get_offset(int field_nb) const
{
	if ((field_nb < 0) || (field_nb >= field_max))
		return -1;
	return fields[field_nb].offset;
}

int ftrace_decode_binary_event::add_field(const char * field_name)
{
	if (used >= field_max)
		return -1;
	if (filename == NULL)
		return -1;
	FILE * fd = fopen(filename, "r");
	if (fd == NULL)
	{
		//printf("PMF sensor ftrace %s:%s not found wrong filename %s\n", name, field_name, filename);
		return -1;
	}
	while (1)
	{
		while (fscanf(fd, "%*[\n]") > 0)
			;
		char line[256];
		if (fscanf(fd, "%[^\n]", line) < 1)
			break;
		char * next = strstr(line, "field:");
		if (next == NULL)
			continue;
		next = strstr(next, field_name);
		if (next == NULL)
			continue;
		next = strstr(next, "offset:");
		if (next == NULL)
			continue;
		fields[used].offset = atol(next + strlen("offset:"));
		next = strstr(next, "size:");
		if (next == NULL)
			continue;
		fields[used].size = atol(next + strlen("size:"));
		next = strstr(next, "signed:");
		if (next == NULL)
			continue;
		fields[used].is_signed = atol(next + strlen("signed:")) & 1;
		fclose(fd);
		used++;
		return used - 1;
	}
	fclose(fd);
	//printf("PMF sensor ftrace %s:%s not found\n", name, field_name);
	return -1;
}

bool ftrace_decode_binary_event::read_string(const char * data, char * string, int string_size, int field_nb) const
{

	if ((field_nb < 0) || (field_nb >= field_max))
		return false;
	if ((string_size < fields[field_nb].size + 1) && (fields[field_nb].size != 0))
		return false;
	if (fields[field_nb].size != 0)
	{
		memcpy(string, data + fields[field_nb].offset, fields[field_nb].size);
		string[fields[field_nb].size] = 0;
	}
	else
	{
		strncpy(string, data + fields[field_nb].offset, string_size);
		string[string_size - 1] = 0;
	}
	return true;
}

unsigned long long ftrace_decode_binary_event::read_number_unsigned(const char * data, int field_nb) const
{
	unsigned long long number = 0;
	if ((field_nb < 0) || (field_nb >= field_max))
		return number;
	switch (fields[field_nb].size)
	{
		case 1:
			number = *((unsigned char *)(data + fields[field_nb].offset));
			break;
		case 2:
			number = *((unsigned short *)(data + fields[field_nb].offset));
			break;
		case 4:
			number = *((unsigned int *)(data + fields[field_nb].offset));
			break;
		case 8:
			number = *((unsigned long long *)(data + fields[field_nb].offset));
			break;
		default:
			break;
	}
	return number;
}

long long ftrace_decode_binary_event::read_number_signed(const char * data, int field_nb) const
{
	long long number = 0;
	if ((field_nb < 0) || (field_nb >= field_max))
		return number;
	switch (fields[field_nb].size)
	{
		case 1:
			number = *((char *)(data + fields[field_nb].offset));
			break;
		case 2:
			number = *((short *)(data + fields[field_nb].offset));
			break;
		case 4:
			number = *((int *)(data + fields[field_nb].offset));
			break;
		case 8:
			number = *((long long *)(data + fields[field_nb].offset));
			break;
		default:
			break;
	}
	return number;
}

bool ftrace_decode_binary_event::is_number_signed(int field_nb) const
{
	if ((field_nb < 0) || (field_nb >= field_max))
		return false;
	return fields[field_nb].is_signed != 0;
}

ftrace_decode_binary_header::ftrace_decode_binary_header()
	: header_page("header_page"),
	field_timestamp(header_page.add_field("timestamp")), field_commit(header_page.add_field("timestamp")),
	field_data(header_page.add_field("data")), data(NULL), time(0), length(0), eob(true),
	size_remaining(0), size_data_remaining(0)
{
}

void ftrace_decode_binary_header::set_buffer(const char * buffer, int size)
{
	if ((field_commit < 0) || (field_timestamp < 0) || (field_data < 0))
		return;
	size_remaining = size;
	data = buffer;
	/* end of buffer */
	if (size_remaining < header_page.get_offset(field_data) + header_compressed_size)
	{
		size_remaining = 0;
		size_data_remaining = 0;
		data = NULL;
		length = 0;
		time = 0;
		eob = true;
		return;
	}

	eob = false;
	time = header_page.read_number_unsigned(data, field_timestamp);
	commit = header_page.read_number_unsigned(data, field_commit);
	/* FIXME: use field_commit */
	data += header_page.get_offset(field_data);
	size_remaining = size_remaining - header_page.get_offset(field_data);
	size_data_remaining = header_page.get_size(field_data);

	if (size_data_remaining > size_remaining)
		size_data_remaining = size_remaining;

	/* first element */
	time +=   (*((unsigned int *)data)) >> 5;
	length = ((*((unsigned int *)data)) & 31) * 4;
	if (length + header_compressed_size > size_data_remaining)
		length = size_data_remaining - header_compressed_size;
}

int ftrace_decode_binary_header::get_length() const
{
	return length;
}

bool ftrace_decode_binary_header::end_of_buffer() const
{
	return eob;
}


unsigned long long ftrace_decode_binary_header::get_time() const
{
	return time;
}

const char * ftrace_decode_binary_header::get_current_event() const
{
	if (data == NULL)
		return NULL;
	return data + header_compressed_size;
}

void ftrace_decode_binary_header::get_next_event()
{
	if (end_of_buffer())
		return;

	/* remove the previous data */
	size_remaining -= length + header_compressed_size;
	size_data_remaining -= length + header_compressed_size;
	data += length + header_compressed_size;

	/* not even space for an header, looks to the next chunk of data */
	if (size_data_remaining < header_compressed_size)
	{
		data += size_data_remaining;
		size_remaining -= size_data_remaining;
		set_buffer(data, size_remaining);
		return;
	}
	commit--;
	time +=   (*((unsigned int *)data)) >> 5;
	length = ((*((unsigned int *)data)) & 31) * 4;
	if (length + header_compressed_size > size_data_remaining)
		length = size_data_remaining - header_compressed_size;
}

ftrace_decode_binary_dispatch::ftrace_decode_binary_dispatch()
	: current_event_data(NULL), current_callback(-1), callback_used(0)
{
}

ftrace_decode_binary_dispatch::~ftrace_decode_binary_dispatch()
{
	while (callback_used > 0)
		unregister(callbacks[0].event->name, callbacks[0].fn);
}

bool ftrace_decode_binary_dispatch::register_callback(const char * event, ftrace_decode_callback_fn fn, void * data, const char ** number, const char ** str)
{
	bool error = false;
	/* validate parametters */
	if (callback_used >= callback_max)
		return false; /* you have to recompile with bigger callback_max */
	callbacks[callback_used].event = new ftrace_decode_binary_event(event);
	if (callbacks[callback_used].event == NULL)
		return false;
	callbacks[callback_used].fn = fn;
	callbacks[callback_used].data = data;
	callbacks_id[callback_used]= callbacks[callback_used].event->id;

	int number_nb = 0;
	int str_nb = 0;
	const char **temp = number;
	if (temp != NULL)
		while (*temp != NULL)
		{
			number_nb++;
			temp++;
		}
	temp = str;
	if (temp != NULL)
		while (*temp != NULL)
		{
			str_nb++;
			temp++;
		}


	callbacks[callback_used].number_index =
		(int *)malloc(number_nb * (sizeof(int) + sizeof(long long))
					+ str_nb * (sizeof(int) + sizeof(const char *)));
	if (callbacks[callback_used].number_index == NULL)
	{
		delete callbacks[callback_used].event;
		return false;
	}
	callbacks[callback_used].number_value = (long long *)(callbacks[callback_used].number_index + number_nb);
	callbacks[callback_used].str_index = (int *)(callbacks[callback_used].number_value + number_nb);
	callbacks[callback_used].str_value = (const char **)(callbacks[callback_used].str_index + str_nb);

	for (int i = 0; i < number_nb; i++)
	{
		callbacks[callback_used].number_index[i] =
			callbacks[callback_used].event->add_field(number[i]);
		if (callbacks[callback_used].number_index[i] == -1)
		{
			error = true;
			break;
		}
	}
	for (int j = 0; j < str_nb; j++)
	{
		callbacks[callback_used].str_index[j] =
			callbacks[callback_used].event->add_field(str[j]);
		if (callbacks[callback_used].str_index[j] == -1)
		{
			error = true;
			break;
		}
	}

	if (error)
	{
		/* some field are unknown, so never call the method */
		free(callbacks[callback_used].number_index);
		delete callbacks[callback_used].event;
		return false;
	}

	callbacks[callback_used].number_nb = number_nb;
	callbacks[callback_used].str_nb = str_nb;
	callback_used++;
	return true;
}

bool ftrace_decode_binary_dispatch::unregister(const char * event, ftrace_decode_callback_fn fn)
{
	int i = 0;
	while (i < callback_used)
	{
		if (callbacks[i].fn != fn)
		{
			i++;
			continue;
		}
		if (strcmp(callbacks[i].event->name, event) != 0)
		{
			i++;
			continue;
		}
		free(callbacks[i].number_index);
		delete callbacks[i].event;
		for (int j = i; j < callback_used - 1; j++)
			callbacks[j] = callbacks[j + 1];
		callback_used--;
		return true;
	}
	return false;
}

void ftrace_decode_binary_dispatch::process_data(void)
{
	char * buffer[cpu_max];
	int size[cpu_max];
	int cpu_nb;
	int cpu_active = 0;
	int cpu;
	ftrace_decode_binary_header * header[cpu_max];
	ftrace_decode_binary_event event_print = ftrace_decode_binary_event("ftrace/print");

	for (cpu_nb = 0; cpu_nb < cpu_max; cpu_nb++)
	{
		buffer[cpu_nb] = (char *)malloc(page_size);
		if (buffer[cpu_nb] == NULL)
			break;
		size[cpu_nb] = cpu_buffer_read(buffer[cpu_nb], page_size, cpu_nb);
		if (size[cpu_nb] <= 0)
		{
			free(buffer[cpu_nb]);
			buffer[cpu_nb] = NULL;
			continue;
		}
		header[cpu_nb] = new ftrace_decode_binary_header();
		if (header[cpu_nb] == NULL)
		{
			free(buffer[cpu_nb]);
			buffer[cpu_nb] = NULL;
			continue;
		}
		header[cpu_nb]->set_buffer(buffer[cpu_nb], size[cpu_nb]);
		cpu_active++;
	}
	if (cpu_nb == 0)
		return;

	do
	{
		int i;
		double time = HUGE_VAL;
		int min = 0;
		for (cpu = 0; cpu < cpu_nb; cpu++)
			if ((buffer[cpu] != NULL) && (header[cpu]->get_time() < time))
			{
				min = cpu;
				time = header[cpu]->get_time();
			}
		cpu = min;
		time = time / 1000.0 / 1000.0 / 1000.0;
		current_event_data = header[cpu]->get_current_event();
		int id = event_print.read_number_unsigned(current_event_data, event_print.field_type);
		for (i = callback_used - 1; i >= 0; i--)
		{
			if (callbacks_id[i] == id)
			{
				bool success = true;
				int temp_str_offset = 0;
				current_callback = i;
				printf("%lf %d\n", time, cpu);
				for (int j = 0; j < callbacks[i].number_nb; j++)
					callbacks[i].number_value[j] =
					callbacks[i].event->read_number_signed(current_event_data,
										callbacks[i].number_index[j]);
				for (int j = 0; j < callbacks[i].str_nb; j++)
				{
					callbacks[i].str_value[j] = NULL;
					success = callbacks[i].event->read_string(current_event_data,
								temp_str + temp_str_offset,
								temp_str_size - temp_str_offset
									- (callbacks[i].str_nb - j) /* at least one space for \000 */,
								callbacks[i].str_index[j]);
					if (!success)
						break;
					temp_str_offset += 1 + strlen(temp_str + temp_str_offset);
				}
				if (success)
					callbacks[i].fn(callbacks[i].data, cpu, time,
						callbacks[i].number_value, callbacks[i].str_value);
			}
		}
		do
		{
			header[cpu]->get_next_event();
			if (!header[cpu]->end_of_buffer())
				break;
			size[cpu] = cpu_buffer_read(buffer[cpu], page_size, cpu);
			if (size[cpu] > 0)
			{
				header[cpu]->set_buffer(buffer[cpu], size[cpu]);
				break;
			}
			cpu_active--;
			free(buffer[cpu]);
			buffer[cpu] = NULL;
			delete header[cpu];
			header[cpu] = NULL;
		} while (0);
	} while (cpu_active > 0);
	current_event_data = NULL;
	current_callback = -1;
}

int ftrace_decode_binary_dispatch::ftrace_decode_get_index(const char * name)
{
	if (current_callback == -1)
		return -1;
	return callbacks[current_callback].event->add_field(name);
}
long long ftrace_decode_binary_dispatch::ftrace_decode_get_value(int index)
{
	if (current_callback == -1)
		return -1;
	return callbacks[current_callback].event->read_number_signed(current_event_data, index);
}
const char * ftrace_decode_binary_dispatch::ftrace_decode_get_str(int index)
{
	if (current_callback == -1)
		return NULL;
	if (callbacks[current_callback].event->read_string(current_event_data, temp_str, temp_str_size, index))
		return temp_str;
	return NULL;
}

bool ftrace_decode_binary_dispatch_file::save(const char * file)
{
	int sum = 0;
	bool nothing;
	int cpu;
	int cpu_nb;
	char temp[256];
	char * buffer = (char *)malloc(page_size);
	int cpu_fd[cpu_max];
	int cpu_size[cpu_max];
	if (buffer == NULL)
		return false;
	fd = fopen(file, "w");
	if (fd == NULL)
	{
		free(buffer);
		return false;
	}
	for (cpu = 0; cpu < cpu_max; cpu++)
	{
		sprintf(temp, "/sys/kernel/debug/tracing/per_cpu/cpu%d/trace_pipe_raw", cpu);
		cpu_fd[cpu] = open(temp, O_RDONLY);
		if (cpu_fd[cpu] < 0)
			break;
		cpu_size[cpu] = 0;
	}
	cpu_nb = cpu;
	fseek(fd, page_size, SEEK_SET);
	do
	{
		nothing = true;
		for (cpu = 0; cpu < cpu_nb; cpu++)
		{
			int size;
			if (cpu_fd[cpu] < 0)
				continue;
			do {
				size = read(cpu_fd[cpu], buffer, page_size);
			} while (size < 0);
			if (size != page_size)
			{
				close(cpu_fd[cpu]);
				cpu_fd[cpu] = -1;
				continue;
			}
			cpu_size[cpu] = cpu_size[cpu] + size;
			sum += fwrite(buffer, 1, size, fd);
			nothing = false;
		}
	}
	while (!nothing);
	fseek(fd, 16, SEEK_SET);
	fwrite(&cpu_nb, sizeof(int), 1, fd);
	fwrite(&cpu_size[0], sizeof(int), cpu_nb, fd);
	fclose(fd);
	free(buffer);
	return true;	
}


bool ftrace_decode_binary_dispatch_file::process(const char * file)
{
	fd = fopen(file, "r");
	if (fd == NULL)
		return false;
	/* do basic check to know if it's a real/valid binary data file */
	do {
		int i;
		long long sum = 0;
		if (fread(&i, sizeof(int), 1, fd) <= 0)
			break;
		if (i != 0)
			break; /* if first byte is 0 it's definitivilly not an ASCII file !*/
		if (fseek(fd, 16, SEEK_SET) != 16)
			break;
		if (fread(&cpu_nb, sizeof(int), 1, fd) <= 0)
			break;
		if (fread(&cpu_size[0], sizeof(int), cpu_nb, fd) <= 0)
			break;
		if ((cpu_nb <= 0) || (cpu_nb >= cpu_max))
			break;
		for (i = 0; i < cpu_nb; i++)
		{
			cpu_size_current[i] = 0;
			sum += cpu_size[i];
		}
		if ((sum & (page_size - 1)) != 0)
			break;
		/* size of file is sum cpu_size + page_size, since the first page_size data
			contains the cpu_nb/cpu_size[] */
		if (fseek(fd, sum, SEEK_SET) != sum)
			break;
		process_data();
		fclose(fd);
		return true;
	} while (0);
	fclose(fd);
	return false;
}

int ftrace_decode_binary_dispatch_file::cpu_buffer_read(char * buffer, int size, int cpu)
{
	int i;
	int offset = page_size;
	if (cpu >= cpu_nb)
		return -1;
	if (cpu_size_current[cpu] >= cpu_size[cpu])
		return -1;
	int row_current = cpu_size_current[cpu] / page_size;
	/* add previous row */
	for (i = 0; i < cpu_nb; i++)
	{
		int row_size = cpu_size[i] / page_size;
		if (row_size >= row_current)
			offset += row_current * page_size;
		else
			offset += row_size * page_size;
	}
	/* add cpu on same row */
	for (i = 0; i < cpu; i++)
	{
		int row_size = cpu_size[i] / page_size;
		if (row_size > row_current)
			offset += page_size;
	}
	fseek(fd, offset, SEEK_SET);
	cpu_size_current[cpu] += page_size;
	return fread(buffer, 1, page_size, fd);
}

