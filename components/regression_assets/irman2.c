/*
 *  irman by Davide Libenzi ( irman load generator )
 *  Copyright (C) 2003  Davide Libenzi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
//#include <sys/signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "./test.h"
#include <sys/wait.h>


#define BUFSIZE (1024 * 32)


static int *pipes;
static int num_pipes, num_active;
static unsigned long burn_ms;
static char buf1[BUFSIZE], buf2[BUFSIZE];


unsigned long long getustime(void) {
	struct timeval tm;

	gettimeofday(&tm, NULL);
	return (unsigned long long) tm.tv_sec * 1000ULL + (unsigned long long) tm.tv_usec / 1000ULL;
}


int burn_ms_cpu(unsigned long ms) {
	int i, cmp = 0;
	unsigned long long ts;

	ts = getustime();
	do {
		for (i = 0; i < 4; i++)
			cmp += memcmp(buf1, buf2, BUFSIZE);
	} while (ts + ms > getustime());
	return cmp;
}


pid_t hog_process(void) {
	pid_t pid;
	int l;

	if (!(pid = fork())) {
		for (l = 0; l < 128; l++) {
			printf("HOG running %u\n", time(NULL));
			burn_ms_cpu(burn_ms);
		}
		exit(0);
	}
	return pid;
}


pid_t irman_process(int n) {
	int nn,k;
	pid_t pid;
	u_char ch;

	if (!(pid = test_children_fork())) {
		if ((nn = n + num_active) >= num_pipes)
			nn -= num_pipes;
		for (k = 0; k < 16; k++) {
			printf("reading %u\n", n);
			read(pipes[2 * n], &ch, 1);
			burn_ms_cpu(burn_ms);
			printf("writing %u\n", nn);
			write(pipes[2 * nn + 1], "s", 1);
		}
		exit(0);
	}
	return pid;
}


int main_irman2(int argc, char **argv)
{
	struct rlimit rl;
	int i, c;
	long work;
	int *cp;
	extern char *optarg;
	int pids[1024];

	num_pipes = 5;
	num_active = 1;
	burn_ms = 300;
	while ((c = getopt(argc, argv, "n:b:a:")) != -1) {
		switch (c) {
		case 'n':
			num_pipes = atoi(optarg);
			break;
		case 'b':
			burn_ms = atoi(optarg);
			break;
		case 'a':
			num_active = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Illegal argument \"%c\"\n", c);
			exit(1);
		}
	}

	rl.rlim_cur = rl.rlim_max = num_pipes * 2 + 50;
	if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
		perror("setrlimit"); 
		exit(1);
	}

	pipes = calloc(num_pipes * 2, sizeof(int));
	if (pipes == NULL) {
		perror("malloc");
		exit(1);
	}

	for (cp = pipes, i = 0; i < num_pipes; i++, cp += 2) {
		if (pipe(cp) == -1) {
			perror("pipe");
			exit(1);
		}
	}

	memset(buf1, 'f', sizeof(buf1));
	memset(buf2, 'f', sizeof(buf2));

	for (i = 0; i < num_pipes; i++)
		pids[i] = irman_process(i);

	pids[num_pipes] = hog_process();

	for (i = 0; i < num_active; i++)
		write(pipes[2 * i + 1], "s", 1);

	return test_children_gc(false);
}

