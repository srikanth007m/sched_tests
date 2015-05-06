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

#define CL_SZ	64
#define PAGE_SZ	4096
#define MEM_SZ	(PAGE_SZ * 1024)

static char mem[MEM_SZ] = {0x55, };

const char usage_text[] = "[-l <1-1024*1024>] [-w] [-d] [-v] [-h]";

const char help_text[]=
		"\n"
		"-l\n"
		"    (outer) loop counter (default 1024).\n"
		"-w\n"
		"    in addition of reading the first byte of each cachline,\n"
		"    write to it too.\n"
		"-d\n"
		"    dry run, do not read or write to any cacheline.\n"
		"    Useful to figure out how many cache misses the program\n"
		"    body produces by itself.\n"
		"-v\n"
		"    verbose output.\n"
		"-h\n"
		"    print this help text.\n"
		"\n"
		"Cache provokes caches misses in L1 data and L2 unified\n"
		"cache.\n"
		"A memory area of 4MB (4096*1024) bytes is allocated in its\n"
		"data segment.\n"
		"The cache sizes of the TC2 chip: A7/A15 L1 data cache is\n"
		"32KB (1/128 of the memory), L2 unified cache of the A7\n"
		"cluster is 256KB (1/16 of the memory) and L2 unified\n"
		"cache of the A15 cluster is 1MB (1/4 of the memory).\n"
		"Cache reads the first byte of each cacheline of the memory\n"
		"in the inner loop producing 65536 cache misses in L1 data\n"
		"cache.\n"
		"This inner loop runs multiple times according to the value\n"
		"of the outer loop.\n"
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
	int c, i, y;
	void *pa;
	char *p;
	unsigned int loopc = 1024, write = 0, dry = 0, verbose = 0;

	while ((c = getopt(argc, argv, "l:wdvh")) != -1) {
		switch (c) {
		case 'l':
			loopc = atoi(optarg);
			break;
		case 'w':
			write = 1;
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

	if (loopc < 1 || loopc > 1024*1024) {
		fprintf(stderr, "loop counter %d not supported\n", loopc);
		exit(1);
	}

	if (verbose)
		fprintf(stdout, "%s%s run with %d loops [%s]\n",
			argv[0], dry ? " dry" : "", loopc,
			write ? "read and write" : "read only");

	pa = mem;

	if (!pa) {
		fprintf(stderr, "Error: mmap failed\n");
		exit(2);
	}

	if (verbose)
		fprintf(stdout, "pa = %p\n", pa);

	for (i = 0; i < loopc; i++) {
		p = pa;
		for (y = 0; y < MEM_SZ/CL_SZ; y++) {
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
			volatile char v = 0;
#pragma GCC diagnostic pop
			if (!dry) {
				v = *p;
				if (write)
					*p = 0xAA;
				p += CL_SZ;
			}
		}
	}

	return 0;
}
