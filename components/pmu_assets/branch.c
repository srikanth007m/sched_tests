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

#define PAGE_SZ 4096
#define MEM_SZ  (PAGE_SZ * 1024)

static unsigned char mem[MEM_SZ] = {0x55, };

const char usage_text[] = "[-l <1-16> [-d] [-v] [-h]";

const char help_text[]=
		"\n"
		"-l\n"
		"    (outer) loop counter (default 1).\n"
		"-d\n"
		"    dry run, do not run the if/else construct.\n"
		"    Useful to figure out how many branch misses the program\n"
		"    body produces by itself.\n"
		"-v\n"
		"    verbose output.\n"
		"-h\n"
		"    print this help text.\n"
		"\n"
		"Branch provokes branch misses.\n"
		"A memory area of 4MB (4096*1024) bytes is allocated in its\n"
		"data segment. It is then filled with pseudo-random values\n"
		"by reading this amount of data from the </dev/urandom>\n"
		"device.\n"
		"For each byte in the memory area branch runs a 5-level\n"
		"if/else construct to find the value of the byte like in a\n"
		"like a divide and conquer strategy.\n"
		"This inner loop runs multiple times according to the value\n"
		"of the outer loop.\n"
		"";

static int fill_buffer(void);
static void do_work(unsigned int verbose, unsigned int dry);

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
	unsigned int loopc = 1, dry = 0, verbose = 0;

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

	if (loopc < 1 || loopc > 16) {
		fprintf(stderr, "loop counter %d not supported\n", loopc);
		exit(1);
	}

	if (verbose)
		fprintf(stdout, "%s%s run with %d loops\n", argv[0], dry ? " dry" : "", loopc);

	for (i = 0; i < loopc; i++) {
		if (fill_buffer())
			exit (2);
		do_work(verbose, dry);
	}

	return 0;
}

int fill_buffer(void) {
	FILE *f;

	if ((f = fopen("/dev/urandom", "r")) == NULL) {
		perror("Error fopen\n");
		return -1;
	}

	if (fread(&mem, sizeof (unsigned char), MEM_SZ, f) != MEM_SZ) {
		perror("Error fread\n");
		return -1;
	}

	if (fclose(f) == EOF) {
		perror("Error fclose\n");
		return -1;
	}

	return 0;
}

void do_work(unsigned int verbose, unsigned int dry) {
	unsigned int i;
	volatile long long v = 0;

	for (i = 0; i < MEM_SZ; i++) {
		if (!dry) {
			if (mem[i] < 0x80)
				if (mem[i] < 0x40)
					if (mem[i] < 0x20)
						if (mem[i] < 0x10)
							if (mem[i] < 0x08)
								v += mem[i];
							else
								v -= mem[i];
						else
							if (mem[i] < 0x18)
								v += mem[i];
							else
								v -= mem[i];
					else
						if (mem[i] < 0x30 )
							if (mem[i] < 0x28)
								v += mem[i];
							else
								v -= mem[i];
						else
							if (mem[i] < 0x38)
								v += mem[i];
							else
								v -= mem[i];
				else
					if (mem[i] < 0x60)
						if (mem[i] < 0x50)
							if (mem[i] < 0x48)
								v += mem[i];
							else
								v -= mem[i];
						else
							if (mem[i] < 0x58)
								v += mem[i];
							else
								v -= mem[i];
					else
						if (mem[i] < 0x70)
							if (mem[i] < 0x58)
								v += mem[i];
							else
								v -= mem[i];
						else
							if (mem[i] < 0x78)
								v += mem[i];
							else
								v -= mem[i];
			else
				if (mem[i] < 0xC0)
					if (mem[i] < 0xA0)
						if (mem[i] < 0x90)
							if (mem[i] < 0x88)
								v += mem[i];
							else
								v -= mem[i];
						else
							if (mem[i] < 0x98)
								v += mem[i];
							else
								v -= mem[i];
					else
						if (mem[i] < 0xB0)
							if (mem[i] < 0xA8)
								v += mem[i];
							else
								v -= mem[i];
						else
							if (mem[i] < 0xB8)
								v += mem[i];
							else
								v -= mem[i];
				else
					if (mem[i] < 0xC0)
						if (mem[i] < 0xD0)
							if (mem[i] < 0xC8)
								v += mem[i];
							else
								v -= mem[i];
						else
							if (mem[i] < 0xD8)
								v += mem[i];
							else
								v -= mem[i];
					else
						if (mem[i] < 0xF0)
							if (mem[i] < 0xE8)
								v += mem[i];
							else
								v -= mem[i];
						else
							if (mem[i] < 0xF8 )
								v += mem[i];
							else
								v -= mem[i];
		}
	}

	if (verbose && !dry)
		fprintf(stdout, "i = %d, v = %lld vm = %lld\n", i, v, v/i);

	return;
}

