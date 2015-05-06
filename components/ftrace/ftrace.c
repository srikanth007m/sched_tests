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

/* ftrace - analyze trace file using analysis libraries */

#include "ftrace_decode.h"
#include "ftrace_helper.h"
#include "ftrace_dl_api.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define HANDLE_MAX 256
int main(int argc, char ** argv)
{
	int i;
	void * handles[HANDLE_MAX];
	bool ret = true;
	bool selected;
	printf("#%s from Olivier Cozette <olivier.cozette@arm.com>\n", argv[0]);
	if (argc == 1)
	{
		printf("Usage\n\t%s [-s file] [-t tracefile1 [-t tracefile2 [-t fracefile3 [...]]]] [-l ./libanalizis1.so [-l ./libanalizis2.so [-l ./libanalizis3.so [...]]]] [-q ./libanalyzis4.so]\n", argv[0]);
		printf("Will analyze all the trace file tracefile1, tracefile2, tracefile3,\n");
		printf("these files could have been created either by cat ../tracing/trace >tracefileor with\n");
		printf("option -s\n");
		printf("\tusing the library libanalizis1.so, libanalizis1.so and libanalizis1.so\n\tand give information on libanalizis4.so\n");
		printf("\tfile is the file where the binary will be saved\n");
		printf("Example\n\t%s -s file_to_store_my_trace\n", argv[0]);
		printf("\t\tStore ftrace in binary format in file_to_store_my_trace\n");		
		printf("\t%s -q ./libanalyzis4.so\n", argv[0]);
		printf("\t\tTell what ./libanalyzis4.so is doing\n");
		printf("\t%s -t tracefile -l ./libanalyzis4.so\n", argv[0]);
		printf("\t\tAnalyze tracefile with Tell what ./libanalyzis4.so\n");
		printf("\t%s [-L]\n\t\tShow all available library internally or in current directory\n", argv[0]);
		printf("\t%s [-LL]\n\t\tShow all available library internally or in current directory and their description\n", argv[0]);
		return 1;
	}

	/* List libs */
	for (i = 1; i < argc; i++)
	{
		struct ftrace_dl_enum * handle;
		const char * libname;
		const char * info;
		if (argv[i][0] != '-')
			continue;
		if ((strcmp(argv[i], "-L") != 0)
			&& (strcmp(argv[i], "-LL") != 0))
		{
			/* skip next arg */
			i++;
			continue;
		}
		printf("Libs:\n");
		handle = ftrace_dl_enum_open();
		if (handle == NULL)
			continue;
		while (ftrace_dl_enum_next(handle, &libname, &info))
		{
			char * info2;
			printf("\t %s (%s)\n", libname, info);
			if (strcmp(argv[i], "-LL") != 0)
				continue;
			info2 = ftrace_dl_getinfo(libname);
			if (info2 == NULL)
				continue;
			printf("\t%s\n", info2);
			free(info2);
		}
		ftrace_dl_enum_close(handle);
	}

	ftrace_decode_init();
	ftrace_helper_init();
	/* check for saving file */
	for (i = 1; i < argc - 1; i++)
	{
		if (strcmp(argv[i], "-s") != 0)
			continue;
		ftrace_decode_save(argv[i + 1]);
	}

	/* Information */
	selected = false;
	for (i = 1; i < argc; i++)
	{
		char * info;
		if (argv[i][0] == '-')
			selected = false;
		if (strcmp(argv[i], "-q") == 0)
		{
			selected = true;
			continue;
		}
		if (!selected)
			continue;
		info = ftrace_dl_getinfo(argv[i]);
		if (info == NULL)
		{
			printf("# %s : Library not found. Notice that you need to use the fulll library path.\n",
				argv[i]);
			continue;
		}
		printf("# %s : %s\n", argv[i], info);
		free(info);
	}

	/* load libraries */
	selected = false;
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
			selected = false;
		if (strcmp(argv[i], "-l") == 0)
		{
			selected = true;
			continue;
		}
		if (!selected)
			continue;
		handles[i] = ftrace_dl_load(argv[i]);
		if (handles[i] != NULL)
			continue;
		printf("# %s : Library not found. Notice that you need to use the fulll library path.\n",
				argv[i]);
	}

	selected = false;
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
			selected = false;
		if (strcmp(argv[i], "-t") == 0)
		{
			selected = true;
			continue;
		}
		if (!selected)
			continue;
		ftrace_decode_process(argv[i]);
	}

	selected = false;
	for (i = 1; i < argc - 1; i++)
	{
		bool ret1;
		if (argv[i][0] == '-')
			selected = false;
		if (strcmp(argv[i], "-l") == 0)
		{
			selected = true;
			continue;
		}
		if (!selected)
			continue;
		ret1 =  ftrace_dl_unload(handles[i]);
		printf("# %s: %s\n", argv[i], ret1?"SUCCESS":"FAILED");
		ret = ret && ret1;
	}

	ftrace_helper_cleanup();
	ftrace_decode_cleanup();
	printf("# Result: %s\n", ret?"SUCCESS":"FAILED");
	return ret?0:1;
}

