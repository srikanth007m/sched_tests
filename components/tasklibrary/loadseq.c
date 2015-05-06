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

/* loadseq - iteration count sequence generation for load generator */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include "timerload.h"

struct loadseq_source
{
	void (*open)(struct loadseq_source *self, void *data);
	void (*get_next_token)(struct loadseq_source *self, const char **token_ptr, int *token_length);
	void (*close)(struct loadseq_source *self);
	void *opaque;
};
void open_file_source(struct loadseq_source *self, void *data);
void get_next_token_file(struct loadseq_source *self, const char **token_ptr, int *token_length);
void close_file_source(struct loadseq_source *self);
void open_string_source(struct loadseq_source *self, void *data);
void get_next_token_string(struct loadseq_source *self, const char **token_ptr, int *token_length);
void close_string_source(struct loadseq_source *self);

struct loadseq_source file_source =
{
	.open = open_file_source,
	.get_next_token = get_next_token_file,
	.close = close_file_source,
	.opaque = NULL
};

struct loadseq_source string_source =
{
	.open = open_string_source,
	.get_next_token = get_next_token_string,
	.close = close_string_source,
	.opaque = NULL
};

struct loadseq_element
{
  int time;
  int cpu;
  int percent;
  int priority;
  struct loadseq_element *to;
};
int generate_loadseq(struct loadseq_source *source, struct loadseq_element **array);
int interpolate_loadseq(int num_elements, struct loadseq_element *parser_array, struct raw_loadseq_element **result_array_ptr);
struct loadseq_element *loadseq_array;
struct raw_loadseq_element *raw_loadseq_array;

void build_loadseq(char *data)
{
	int num_entries;
	if(!read_calibration())
	{
		printf("Could not load calibration data\n");
		return;
	}

	if(strlen(data) >= 10)
	{
		string_source.open(&string_source, data + 10);
		num_entries = generate_loadseq(&string_source, &loadseq_array);
		if(num_entries)
			num_entries = interpolate_loadseq(num_entries, loadseq_array, &raw_loadseq_array);
		free(loadseq_array);
		loadseq_to_run = num_entries;
	}
}
void build_loadseq_file(char *data)
{
	int num_entries;
	if(!read_calibration())
	{
		printf("Could not load calibration data\n");
		return;
	}

	if(strlen(data) >= 14)
	{
		file_source.open(&file_source, data+14);
		num_entries = generate_loadseq(&file_source, &loadseq_array);
		if(num_entries)
			num_entries = interpolate_loadseq(num_entries, loadseq_array, &raw_loadseq_array);
		free(loadseq_array);
		loadseq_to_run = num_entries;
	}
}


/*
 * string source
 */
#define STRING_MAGIC 0x12345678
#define FILE_MAGIC 0x87654321
struct string_source_parser
{
	unsigned int magic;
	void *data; /* original string */
	char *string; /* copy of string for parsing, modified as parsed */
	char *curr_ptr; /* current position in string */
	int  setup;
};
void open_string_source(struct loadseq_source *self, void *data)
{
	self->opaque = malloc(sizeof(struct string_source_parser));
	if(self->opaque)
	{
		int datalen = strlen(data);
		struct string_source_parser *obj = self->opaque;
		obj->data = data;
		obj->string = malloc(datalen + 1);
		strncpy(obj->string, data, datalen);
		obj->string[datalen] = '\0';
		obj->curr_ptr = obj->string;
		obj->magic = STRING_MAGIC;
		obj->setup = 1;
	}
}

void get_next_token_string(struct loadseq_source *self, const char **token_ptr, int *token_length)
{
	struct string_source_parser *obj = self->opaque;
	if(obj->magic != STRING_MAGIC)
	{
		printf("ERROR: Corrupted Parser Object. Expected STRING parser 0x%08x, got 0x%08x\n", STRING_MAGIC, obj->magic);
		exit(EXIT_FAILURE);
	}
	char *next_token;
	if(obj->setup)
	{
		next_token = strtok( obj->string, ":\r\n");
		obj->setup = 0;
	} else {
		next_token = strtok( NULL, ":\r\n");
	}

	*token_ptr = next_token;
	*token_length = next_token ? strlen(next_token) : 0;
}

void close_string_source(struct loadseq_source *self)
{
	struct string_source_parser *obj = self->opaque;
	if(obj->magic != STRING_MAGIC)
	{
		printf("ERROR: Corrupted Parser Object. Expected STRING parser 0x%08x, got 0x%08x\n", STRING_MAGIC, obj->magic);
		exit(EXIT_FAILURE);
	}
	free(obj->string);
	free(self->opaque);
	self->opaque = NULL;
}

/*
 * file source
 */
#define LINE_BUFFER_LEN 4096
#define TOKEN_BUFFER_LEN 128
struct file_source_parser
{
	unsigned int magic;
	char *filename;
	FILE *infile;
	int eof;
	char token_buffer[TOKEN_BUFFER_LEN];
	char line_buffer[LINE_BUFFER_LEN];
};

void open_file_source(struct loadseq_source *self, void *data)
{
	self->opaque = malloc(sizeof(struct file_source_parser));
	if(self->opaque)
	{
		struct file_source_parser *obj = self->opaque;
		obj->filename = data;
		obj->magic = FILE_MAGIC;
		obj->eof = 0;
		memset(obj->line_buffer, 0, LINE_BUFFER_LEN);
		obj->infile = fopen(obj->filename, "r");
		if(!obj->infile)
		{
			printf("ERROR: Failed to open file %s\n", obj->filename);
			exit(EXIT_FAILURE);
		}
	} else {
		printf("ERROR: Failed to allocate file parser\n");
		exit(EXIT_FAILURE);
	}
}

void fill_input_buffer(struct file_source_parser *obj)
{
	char *pos;
	if(!fgets(obj->line_buffer,LINE_BUFFER_LEN,obj->infile))
		obj->eof = 1;
	/* skip comment lines, starting with # */
	if(obj->line_buffer[0] == '#')
	{
		printf("Comment line: %s\n", obj->line_buffer);
		fill_input_buffer(obj);
		return;
	}
	/* keeping this line */
	/* replace line breaks with NULLs */
	while(pos = strrchr(obj->line_buffer, '\r'), pos)
	{
		*pos = '\0';
	}
	while(pos = strrchr(obj->line_buffer, '\n'), pos)
	{
		*pos = '\0';
	}
}

void get_next_token_file(struct loadseq_source *self, const char **token_ptr, int *token_length)
{
	struct file_source_parser *obj = self->opaque;
	char *readbuf, *writebuf;
	int curr_len, writechars, readchars;
	if(obj->magic != FILE_MAGIC)
	{
		printf("ERROR: Corrupted Parser Object. Expected FILE parser 0x%08x, got 0x%08x\n", FILE_MAGIC, obj->magic);
		exit(EXIT_FAILURE);
	}
	curr_len = strlen(obj->line_buffer);
	if(!curr_len)
		fill_input_buffer(obj);
	curr_len = strlen(obj->line_buffer);
	if(!curr_len || obj->eof)
	{
		*token_ptr = NULL;
		*token_length = 0;
		return;
	}
	readbuf = obj->line_buffer;
	writebuf = obj->token_buffer;
	writechars = 0;
	readchars = 0;
	while(readbuf && readchars < LINE_BUFFER_LEN) {
		readchars++;
		if(*readbuf == '\n' || *readbuf == '\r' || *readbuf == ':' || !*readbuf)
		{
			readbuf++;
			/* end of token, skip initial colons or line breaks */
			if(writechars)
				break;
		} else {
			*writebuf++ = *readbuf++;
			writechars++;
		}
	};
	if(readchars < curr_len)
	{
		/* some stuff left in buffer */
		memmove(obj->line_buffer, readbuf, LINE_BUFFER_LEN - readchars);
	} else {
		obj->line_buffer[0] = '\0';
	}
	obj->token_buffer[writechars] = '\0';
	*token_ptr = obj->token_buffer;
	*token_length = writechars;
}

void close_file_source(struct loadseq_source *self)
{
	struct file_source_parser *obj = self->opaque;
	if(obj->magic != FILE_MAGIC)
	{
		printf("ERROR: Corrupted Parser Object. Expected FILE parser 0x%08x, got 0x%08x\n", FILE_MAGIC, obj->magic);
		exit(EXIT_FAILURE);
	}
	fclose(obj->infile);
	free(self->opaque);
	self->opaque = NULL;
}


/*
 * parser
 */
int big, little;

void parse_token(const char *token, int token_length, struct loadseq_element *element)
{
	/* token is of form <a>,<b>.<c> for single item with optional trailing .<d>
	 * range items are split earlier
	 */
	char temp_buffer[128];
	char *comma=strchr(token,',');
	char *dot=strchr(token,'.');
	static int holding_priority = -100;
	element->cpu = -1;

	if(holding_priority == -100)
		holding_priority = getpriority(PRIO_PROCESS,0);

	element->priority = holding_priority;

	if(!comma) {
		return;
	}
	if(!dot) {
		dot = (char *)(token) + token_length;
	}
	strncpy(temp_buffer, token, comma - token);
	temp_buffer[comma-token]='\0';

	element->time = strtol(temp_buffer, NULL, 10);
	if(dot - comma >= 2)
	{
		int length = (dot - comma) - 1;
		strncpy(temp_buffer, comma+1, length);
		temp_buffer[length] = '\0';
		element->cpu = convert_cpuchar_to_id(temp_buffer, big, little);
	}

	element->percent = strtol(dot+1, NULL, 10);

	/* look for optional priority */
	dot=strchr(dot+1,'.');
	if(dot)
	{
		element->priority = strtol(dot+1, NULL, 10);
		holding_priority  = element->priority;
	}
}
#define INTERMEDIATE_ELEMENT_ALLOCS 32
#define INTERMEDIATE_INTERPOLATE_STEPS 10

#define IM_CHECK (INTERMEDIATE_INTERPOLATE_STEPS+1)
#if IM_CHECK >= INTERMEDIATE_ELEMENT_ALLOCS
#error "INTERMEDIATE_INTERPOLATE_STEPS+1 must be less than INTERMEDIATE_ELEMENT_ALLOCS"
#endif

void check_intermediate_array(struct loadseq_element **array, int new_size, int *existing_alloc_size)
{
	struct loadseq_element *tmp;
	if(new_size >= *existing_alloc_size || !*array)
	{
		*existing_alloc_size += INTERMEDIATE_ELEMENT_ALLOCS;
		tmp = realloc(*array, (*existing_alloc_size * sizeof(struct loadseq_element)));
		if(!tmp) {
			printf("ERROR: Failed to allocate loadseq_element memory for %d objects\n", *existing_alloc_size);
			exit(EXIT_FAILURE);
		}
		*array = tmp;
	}
}

int generate_loadseq(struct loadseq_source *source, struct loadseq_element **ret_array)
{
	int token_length;
	const char *token;
	struct loadseq_element *array=NULL;
	int num_elements=0, i, allocated_elements=0;
	*ret_array=NULL;

	get_big_little_cpuids(&big, &little);
	do{
		source->get_next_token(source, &token, &token_length);
		if(token)
		{
			char *second_part = strchr(token, '-');

			/* if we have a priority present, then we may have a negative nice value
			 * so if our first - char is immediately preceded by a dot, consider it
			 * to be a nice value instead and continue looking for another -
			 */
			if(second_part && *(second_part-1)=='.')
				second_part = strchr(second_part+1, '-');

			if(second_part)
			{
				*second_part=0;
				second_part++;
				check_intermediate_array(&array, num_elements + 2, &allocated_elements);
				memset(&array[num_elements], 0, 2 * sizeof(struct loadseq_element));
				parse_token(token, token_length, &array[num_elements]);
				array[num_elements].to = &array[num_elements+1];
				parse_token(second_part, strlen(second_part), &array[num_elements+1]);
				num_elements+=2;
			} else {
				check_intermediate_array(&array, num_elements + 1, &allocated_elements);
				memset(&array[num_elements], 0, sizeof(struct loadseq_element));
				parse_token(token, token_length, &array[num_elements]);
				num_elements++;
			}
		}
	}while(token);
	source->close(source);

	/* check that we only have 1 unknown CPU (to indicate end point) */
	for(i=0;i<num_elements;i++)
	{
		if((i+1 != num_elements) && (array[i].cpu == -1 || (array[i].to && array[i].to->cpu == -1)))
		{
			printf("ERROR: Unknown CPU calibration point found before end of data\n");
			exit(EXIT_FAILURE);
		}
	}

	*ret_array = array;
	return num_elements;
}

#define OUTPUT_ELEMENT_ALLOCS 32
#define OUTPUT_INTERPOLATE_STEPS 10

#define OP_CHECK (OUTPUT_INTERPOLATE_STEPS+1)
#if OP_CHECK >= OUTPUT_ELEMENT_ALLOCS
#error "OUTPUT_INTERPOLATE_STEPS+1 must be less than OUTPUT_ELEMENT_ALLOCS"
#endif

void check_output_array(struct raw_loadseq_element **array, int new_size, int *existing_alloc_size)
{
	struct raw_loadseq_element *tmp;
	if(new_size >= *existing_alloc_size || !*array)
	{
		*existing_alloc_size += OUTPUT_ELEMENT_ALLOCS;
		tmp = realloc(*array, (*existing_alloc_size * sizeof(struct raw_loadseq_element)));
		if(!tmp) {
			printf("ERROR: Failed to allocate loadseq_element memory for %d objects\n", *existing_alloc_size);
			exit(EXIT_FAILURE);
		}
		*array = tmp;
	}
}

int interpolate_loadseq(int num_elements, struct loadseq_element *parser_array, struct raw_loadseq_element **result_array_ptr)
{
	int index, output_elements=0, allocated_output_elements=0;
	struct raw_loadseq_element *rawout=NULL;
	*result_array_ptr = NULL;
	if(!parser_array || !num_elements)
		return -1;

	for(index=0; index < num_elements; index++)
	{
		if(!parser_array[index].to || (parser_array[index].to && parser_array[index].to->time <= parser_array[index].time))
		{
			/* single item output */
			check_output_array(&rawout, output_elements+1, &allocated_output_elements);
			rawout[output_elements].time = (parser_array[index].time / MILLISECONDS_PER_LOOP);
			rawout[output_elements].loops = get_loop_amount(parser_array[index].cpu, parser_array[index].percent);
			rawout[output_elements].priority = parser_array[index].priority;
			output_elements++;
		} else {
			/* interpolated output */
			int interpolated;
			int time_step,percent_step;
			int time,percent,priority;
			check_output_array(&rawout, output_elements + OUTPUT_INTERPOLATE_STEPS + 1, &allocated_output_elements);
			/* check the right number of steps is performed */
			time_step = (parser_array[index].to->time - parser_array[index].time) / OUTPUT_INTERPOLATE_STEPS;
			percent_step = (parser_array[index].to->percent - parser_array[index].percent) / OUTPUT_INTERPOLATE_STEPS;
			time = parser_array[index].time / MILLISECONDS_PER_LOOP;
			percent = parser_array[index].percent;
			priority = parser_array[index].priority;
			for(interpolated=0;interpolated < OUTPUT_INTERPOLATE_STEPS;interpolated++) {
				rawout[output_elements + interpolated].time = time / MILLISECONDS_PER_LOOP;
				rawout[output_elements + interpolated].loops = get_loop_amount(parser_array[index].cpu, percent);
				rawout[output_elements + interpolated].priority = priority;
				time += time_step;
				percent += percent_step;
			}
			rawout[output_elements + OUTPUT_INTERPOLATE_STEPS].time = (parser_array[index].to->time / MILLISECONDS_PER_LOOP);
			rawout[output_elements + OUTPUT_INTERPOLATE_STEPS].loops = get_loop_amount(parser_array[index].cpu, parser_array[index].to->percent);
			rawout[output_elements + OUTPUT_INTERPOLATE_STEPS].priority = priority;
			output_elements += (OUTPUT_INTERPOLATE_STEPS + 1);
		}
		/* skip embedded range element */
		if(parser_array[index].to != NULL)
			index++;
	}
	/* we need one spare alloc at the end to terminate */
	check_output_array(&rawout, output_elements + 1, &allocated_output_elements);
	rawout[output_elements].time = 0x7FFFFFFF;
	rawout[output_elements].loops = 0;
	output_elements++;

	if(output_elements > 0)
		*result_array_ptr = rawout;
	else
		free(rawout);

	return output_elements;
}
