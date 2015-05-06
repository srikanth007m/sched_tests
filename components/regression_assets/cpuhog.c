/*
 *  cpuhog  by Davide Libenzi ( linux kernel scheduler latency tester )
 *  Version 0.12 - Copyright (C) 2001  Davide Libenzi
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
 *
 *  The purpose of this tool is to create a set of processes that are going
 *  to load the runqueue with different cache drain values.
 *  Build:
 *
 *  gcc -o cpuhog cpuhog.c
 *
 *  Use:
 *
 *  cpuhog [--ntasks n] [--ttime s] [--size b]
 *
 *  --ntask    = Set the number of tasks ( runqueue length )
 *  --ttime    = Set the time cpuhog should run in seconds
 *  --size     = Set the cache drain size in Kb
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "test.h"


#define CACHELINE_SIZE	32
#define MAX_CPUHOG_TASKS	1024
#define STD_NTASKS	1
#define STD_TTIME	8
#define WAIT_NICE	15


pid_t pids[MAX_CPUHOG_TASKS];
volatile int spin = 0;
int datasize = 0;
int *data = NULL;


/*
 * cacheload() code comes from Larry McVoy LMBench
 * Bring howmuch data into the cache, assuming that the smallest cache
 * line is 16/32 bytes.
 */
int cacheload(int howmuch) {
	int done, sum = 0;
	register int *d = data;

#if CACHELINE_SIZE == 16

#define ASUM  sum+=d[0]+d[4]+d[8]+d[12]+d[16]+d[20]+d[24]+d[28]+\
		d[32]+d[36]+d[40]+d[44]+d[48]+d[52]+d[56]+d[60]+\
		d[64]+d[68]+d[72]+d[76]+d[80]+d[84]+d[88]+d[92]+\
		d[96]+d[100]+d[104]+d[108]+d[112]+d[116]+d[120]+d[124];\
		d+=128;	/* ints; bytes == 512 */

#elif CACHELINE_SIZE == 32

#define ASUM  sum+=d[0]+d[8]+d[16]+d[24]+d[32]+d[40]+d[48]+d[56]+\
		d[64]+d[72]+d[80]+d[88]+d[96]+d[104]+d[112]+d[120];\
		d+=128;	/* ints; bytes == 512 */

#else

#define	ASUM	sum+=d[0]+d[1]+d[2]+d[3]+d[4]+d[5]+d[6]+d[7]+d[8]+d[9]+\
		d[10]+d[11]+d[12]+d[13]+d[14]+d[15]+d[16]+d[17]+d[18]+d[19]+\
		d[20]+d[21]+d[22]+d[23]+d[24]+d[25]+d[26]+d[27]+d[28]+d[29]+\
		d[30]+d[31]+d[32]+d[33]+d[34]+d[35]+d[36]+d[37]+d[38]+d[39]+\
		d[40]+d[41]+d[42]+d[43]+d[44]+d[45]+d[46]+d[47]+d[48]+d[49]+\
		d[50]+d[51]+d[52]+d[53]+d[54]+d[55]+d[56]+d[57]+d[58]+d[59]+\
		d[60]+d[61]+d[62]+d[63]+d[64]+d[65]+d[66]+d[67]+d[68]+d[69]+\
		d[70]+d[71]+d[72]+d[73]+d[74]+d[75]+d[76]+d[77]+d[78]+d[79]+\
		d[80]+d[81]+d[82]+d[83]+d[84]+d[85]+d[86]+d[87]+d[88]+d[89]+\
		d[90]+d[91]+d[92]+d[93]+d[94]+d[95]+d[96]+d[97]+d[98]+d[99]+\
		d[100]+d[101]+d[102]+d[103]+d[104]+\
		d[105]+d[106]+d[107]+d[108]+d[109]+\
		d[110]+d[111]+d[112]+d[113]+d[114]+\
		d[115]+d[116]+d[117]+d[118]+d[119]+\
		d[120]+d[121]+d[122]+d[123]+d[124]+d[125]+d[126]+d[127];\
		d+=128;	/* ints; bytes == 512 */

#endif

#define	ONEKB	ASUM ASUM

	for (done = 0; done < howmuch; done += 1024) {
		ONEKB
	}
	return sum;
}


void cpu_hog(void) {
	for (;;) {
		cacheload(datasize);
	}
}

void sig_hup(int sig) {
	++spin;
	signal(SIGHUP, sig_hup);
}


int main_cpuhog(int argc, char ** argv) {
	int ii, silent = 0, ntasks = STD_NTASKS, ttime = STD_TTIME;

	for (ii = 1; ii < argc; ii++) {
		if (strcmp(argv[ii], "--ntasks") == 0) {
			if (++ii < argc) {
				ntasks = atoi(argv[ii]);
				if (ntasks > MAX_CPUHOG_TASKS)
					ntasks = MAX_CPUHOG_TASKS;
			}
			continue;
		}
		if (strcmp(argv[ii], "--ttime") == 0) {
			if (++ii < argc)
				ttime = atoi(argv[ii]);
			continue;
		}
		if (strcmp(argv[ii], "--size") == 0) {
			if (++ii < argc)
				datasize = atoi(argv[ii]) * 1024;
			continue;
		}
		if (strcmp(argv[ii], "--silent") == 0) {
			silent = 1;
			continue;
		}
	}

	if (datasize)
		data = (int *) malloc(datasize);

	signal(SIGHUP, sig_hup);

	for (ii = 0; ii < ntasks; ii++) {
		pids[ii] = test_children_fork();
		if (pids[ii] == -1) {
			perror("fork");
			return 1;
		}
		if (pids[ii] == 0) {
			signal(SIGHUP, sig_hup);
			nice(WAIT_NICE);
			while (!spin);
			nice(-WAIT_NICE);
			cpu_hog();
			exit(0);
		}
	}

	for (ii = 0; ii < ntasks; ii++) {
		if (!silent) fprintf(stdout, "%u\n", (unsigned) pids[ii]);
		if (kill(pids[ii], SIGHUP))
			perror("SIGHUP");
	}

	sleep(ttime);

	for (ii = 0; ii < ntasks; ii++)
		if (kill(pids[ii], SIGKILL))
			perror("SIGKILL");

	return test_children_gc(false);
}

