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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ftrace_decode_ascii.h"

char * ftrace_decode_ascii::skip_space(char * line)
{
	while (*line == ' ')
	{
		*line = 0;
		line++;
	}
	return line;
}

char * ftrace_decode_ascii::get_event(char * line)
{
	char * start = line;
	while ((*line != ':') && (*line != 0))

	{
		if (*line == ' ')
			start = line;
		line++;
	}
	if (*line == 0)
		return NULL;
	*start = 0;
	*line = 0;
	event_current.event_name = start + 1;
	return line + 1;
}

char * ftrace_decode_ascii::get_param(char * line)
{
	char * end;
	event_current.params[event_current.param_nb].name = line;
	while ((*line != '=') && (*line != ' ') && (*line != 0))
		line++;
	if (*line == 0)
		return NULL;
	if (*line != '=')
		return line;
	*line = 0;
	line++;
	event_current.params[event_current.param_nb].str = line;
	event_current.params[event_current.param_nb].value = strtod(line, &end);
	line = end;
	event_current.param_nb++;
	while ((*line != ' ') && (*line != 0))
		line++;
	if (*line == 0)
		return NULL;
	*line = 0;
	return line + 1;	
}

bool ftrace_decode_ascii::get_timecpu(void)
{
	char * line = ftrace_decode_line;
	double n1;
	char * end;
	event_current.param_nb = 0;
	if (*line == 0)
		return false;
	line = strstr(line, " [");
	if (line == NULL)
		return false;
	line += 2;
	n1 = strtod(line, &end);
	if (end == line)
		return false;
	if (*end != ']')
		return false;
	event_current.cpu = (long long)n1;
	end++;
	line = end;
	while ((*line != 0) && ((*line < '0') || (*line > '9') || (*(line -1) != ' ')))
		line++;
	if (*line == 0)
		return false;
	if ((line[0] == '0') && (line[1] == 'x'))
	{
		line += 2;
		n1 = strtol(line, &end, 16);
	}
	else
		n1 = strtod(line, &end);
	if (end == line)
		return false;
	line = end;
	if (*line == 0)
		return false;
	line++;
	event_current.time = n1;
	if (*line == ':')
		line++;

	line = get_event(line);
	if (line == NULL)
		return false;

	while (line != NULL)
	{
		line = skip_space(line);
		line = get_param(line);
	}
	return true;

}

bool ftrace_decode_ascii::process(const char * file)
{
	int nb = 0;
	FILE * fd =fopen(file, "r");
	if (fd == NULL)
		return false;
	/* check if this is an ascii trace file */
	while (1)
	{
		while (fscanf(fd, "%*[\n]") > 0)
			;
		if (fscanf(fd, "%1048576[^\n]", ftrace_decode_line) < 1)
			return false;
		if (strncmp(ftrace_decode_line, "version =", strlen("version =")) == 0)
			continue; /* from trace-cmd */
		if (strncmp(ftrace_decode_line, "cpus=", strlen("cpus=")) == 0)
			continue; /* from trace-cmd */
		if (strncmp(ftrace_decode_line, "CPU ", strlen("CPU ")) == 0)
			continue; /* from trace-cmd */
		if (ftrace_decode_line[0] == '#')
			continue; /* comments at start */
		break;
	}
	if (!get_timecpu())
		return false;
	callback_call();
	nb++;	
	while (1)
	{
		while (fscanf(fd, "%*[\n]") > 0)
			;
		if (fscanf(fd, "%1048576[^\n]", ftrace_decode_line) < 1)
			break;
		if (get_timecpu())
			callback_call();
		nb++;
	}
	fclose(fd);
	fflush(stdout);
	return true;
}

ftrace_decode_ascii::ftrace_decode_ascii(void)
{
	int i;
	for (i = 0; i < events_max; i++)
	{
		events[i].event_name = NULL;
		events[i].callback = NULL;
	}
}

ftrace_decode_ascii::~ftrace_decode_ascii(void)
{
	int i;
	for (i = 0; i < events_max; i++)
	{
		while (events[i].event_name != NULL)
			unregister(events[i].event_name, events[i].callback->fn);
	}
}

bool ftrace_decode_ascii::register_callback(const char * event, ftrace_decode_callback_fn fn, void * data, const char ** number, const char ** str)
{
	int empty = events_max;
	int i;
	for (i = 0; i < events_max; i++)
	{
		if ((events[i].event_name == NULL) && (empty == events_max))
			empty = i;
		if (events[i].event_name == NULL)
			continue;
		if (strcmp(events[i].event_name, event) == 0)
			break;
	}

	if (i == events_max)
	{
		if (empty == events_max)
			return false;
		i = empty;
		events[i].callback = NULL; /*not needed */
		events[i].event_name = strdup(event);
		if (events[i].event_name == NULL)
			return false;
	}

	int number_nb = 0;
	const char ** temp = number;
	if (temp != NULL)
	{
		while (*temp != NULL)
		{
			number_nb++;
			temp++;
		}
	}
	int str_nb = 0;
	temp = str;
	if (temp != NULL)
	{
		while (*temp != NULL)
		{
			str_nb++;
			temp++;
		}
	}

	struct callback_struct *callback =
		(struct callback_struct *)
			malloc(sizeof(struct callback_struct)
				+ number_nb * (sizeof(int)+sizeof(long long))
				+ str_nb * (sizeof(int) + sizeof(const char *)));
	if ((callback == NULL) && (events[i].callback != NULL))
		return false;
	if ((callback == NULL) && (events[i].callback == NULL))
	{
		free(events[i].event_name);
		events[i].event_name = NULL;
		return false;
	}

	callback->data = data;
	callback->fn = fn;
	callback->number = number;
	callback->number_index = (int *)(callback + 1);
	callback->number_value = (long long *)(callback->number_index + number_nb);
	callback->str = str;
	callback->str_index = (int *)(callback->number_value + number_nb);
	callback->str_value = (const char **)(callback->str_index + str_nb);
	callback->next = events[i].callback;
	events[i].callback = callback;

	int j;
	for (j = 0; j < number_nb; j++)
		callback->number_index[j] = -1;
	for (j = 0; j < str_nb; j++)
		callback->str_index[j] = -1;
	return true;
}

bool ftrace_decode_ascii::unregister(const char * event, ftrace_decode_callback_fn fn)
{
	for (int i = 0; i < events_max; i++)
	{
		if (events[i].event_name == NULL)
			continue;
		if (strcmp(events[i].event_name, event) != 0)
			continue;
		struct callback_struct *callback = events[i].callback;
		struct callback_struct *callback_prev = NULL;
		while (callback != NULL)
		{
			if (callback->fn != fn)
			{
				callback_prev = callback;
				callback = callback->next;
				continue;
			}
			if (callback_prev == NULL)
			{
				events[i].callback = callback->next;
				free(events[i].event_name);
				events[i].event_name = NULL;
			}
			else
				callback_prev = callback->next;
			free(callback);
			return true;
		}
	}
	return false;
}

bool ftrace_decode_ascii::callback_call(void)
{
	int j;
	for (j = 0; j < events_max; j++)
	{
		if (events[j].event_name == NULL)
			continue;
		if (strcmp(events[j].event_name, event_current.event_name) == 0)
			break;;
	}
	if (j == events_max)
		return false;
	struct callback_struct *callback = events[j].callback;
	while (callback != NULL)
	{
		int i = 0;
		bool success = true;
		if (callback->number != NULL)
			while (callback->number[i] != NULL)
			{
				if (callback->number_index[i] == -1)
				{
					callback->number_index[i] = get_index(callback->number[i]);
					if (callback->number_index[i] == -1)
					{
						success = false;
						break;
					}
				}
				callback->number_value[i] = get_value(callback->number_index[i]);
				i++;
			}
		i = 0;
		if (callback->str != NULL)
			while (callback->str[i] != NULL)
			{
				if (callback->str_index[i] == -1)
				{
					callback->str_index[i] = get_index(callback->str[i]);
					if (callback->str_index[i] == -1)
					{
						success = false;
						break;
					}
				}
				callback->str_value[i] = get_str(callback->str_index[i]);
				if (callback->str_value[i] == NULL)
				{
					success = false;
					break;
				}
				i++;
			}
		/* if some fields are unknown, don't call the method */
		if (success)
			callback->fn(callback->data,
					event_current.cpu,
					event_current.time,
					callback->number_value,
					callback->str_value);
		callback = callback->next;
	}
	return true;
}

int ftrace_decode_ascii::get_index(const char * name)
{
	int i = 0;
	while ((i < event_current.param_nb)
		&& (strcmp(event_current.params[i].name, name) != 0))
		i++;
	if (i == event_current.param_nb)
		return -1;
	return i;
}

long long ftrace_decode_ascii::get_value(int index)
{
	if ((index < 0) || (index >= event_current.param_nb))
		return 0;
	return event_current.params[index].value;
}

const char * ftrace_decode_ascii::get_str(int index)
{
	if ((index < 0) || (index >= event_current.param_nb))
		return NULL;
	return event_current.params[index].str;
}

