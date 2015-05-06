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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

const char *usage_text = "[-l <1-INT_MAX>] [-d] [-v] [-h]";

const char help_text[]=
		"\n"
		"-l\n"
		"    loop counter (default 256*1024*1024).\n"
		"-d\n"
		"    dry run, do not run the loop.\n"
		"    Useful to figure out how many cycles the program\n"
		"    body produces by itself.\n"
		"-v\n"
		"    verbose output.\n"
		"-h\n"
		"    print this help text.\n"
		"\n"
		"Cycles runs a nop instruction in a loop.\n"
		"";

void usage(char **argv)
{
	fprintf(stderr, "Usage: %s %s\n", argv[0], usage_text);
	exit(-1);
}

void help(char **argv)
{
	fprintf(stderr, "Usage: %s %s\n", argv[0], usage_text);
	fprintf(stderr, "%s\n", help_text);
	exit(-1);
}

int main(int argc, char **argv)
{
	int c, i;
	unsigned int loopc = 256*1024*1024, dry = 0, verbose = 0;

	while ((c = getopt(argc, argv, "l:dvh")) != -1) {
		switch (c) {
		case 'l':
			loopc = atoi(optarg);
			break;
		case 'd':
			dry = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			help(argv);
			break;
		default:
			usage(argv);
			break;
		}
	}

	if (loopc < 1 || loopc > INT_MAX) {
		fprintf(stderr, "loop counter %d not supported\n", loopc);
		exit(1);
	}

	if (verbose)
		fprintf(stdout, "%s%s run with %d loops\n",
			argv[0], dry ? " dry" : "", loopc);

	if (!dry)
		for (i = 0; i < loopc; i++)
			asm("nop");

	return 0;
}
