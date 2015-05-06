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

/* main - entry point and command line parser for load generator */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include "timerload.h"

/* pointer to global calibration data */
struct cpu_data *calibration_data;
/* number of CPUs seen in the calibration data */
int calibration_num_cpus;


void help(char *data)
{
	printf("%s\n", help_text);
}

void calibrate_loops(char *data)
{
	do_calibration();
}

void set_calibfile(char *data)
{
	if(strlen(data) > 12)
	{
		strncpy(calibfile_buffer, data+12, 4096);
		calibfile_buffer[4095] = '\0';
		calibration_file_name = calibfile_buffer;
	}
}

void display_calibration(char *data)
{
	if(!read_calibration())
	{
		printf("Could not load calibration data\n");
		return;
	}

	if(calibration_data)
	{
		int i, big, little, idx=0;
		get_big_little_cpuids(&big, &little);
		printf("Big CPU (b) will be CPU%d\nLittle CPU (l) will be CPU%d\n", big, little);

		while(calibration_data[idx].calibration_points)
		{
			printf("\nCPU%d", idx);
			for(i=0;i<calibration_data[idx].calibration_points;i++)
			{
				struct loops_percent *curr = &calibration_data[idx].data[i];
				printf(",%d%%,%d", curr->percent, curr->loops);
			}
			idx++;
		}
	}
	printf("\n");
}

struct option_list {
	char option_name[32];
	const char *option_desc;
	void (*fn)(char *data);
};

const struct option_list options[] =
{
		{ "--help", help_desc, help },
		{ "--calibrate", calibrate_desc, calibrate_loops },
		{ "--calibfile", calibfile_desc, set_calibfile },
		{ "--display", display_desc, display_calibration },
		{ "--loadseq", loadseq_desc, build_loadseq },
		{ "--fileloadseq", loadseqfile_desc, build_loadseq_file },
		{ "" , NULL , NULL }
};

void usage()
{
	int idx=0;
	printf("tasklibrary CPU load generator\n");
	printf("Options:\n");
	while(options[idx].fn)
	{
		const char empty_string[]="";
		const char *option = options[idx].option_name ? options[idx].option_name : empty_string;
		const char *desc   = options[idx].option_desc ? options[idx].option_desc : empty_string;
		printf("  %s\n    %s\n", option, desc);
		idx++;
	}
}


int run_whatever()
{
	if(loadseq_to_run>0)
		run_loadseq();
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	int arg,idx;
	if(argc <= 1)
	{
		usage();
		return EXIT_FAILURE;
	}
	for(arg=1;arg<argc;arg++)
	{
		idx=0;
		while(options[idx].option_name[0] != 0)
		{
			if(!strncmp(argv[arg], options[idx].option_name, strlen(options[idx].option_name)))
				options[idx].fn(argv[arg]);
			idx++;
		}
	}
	return run_whatever();
}
