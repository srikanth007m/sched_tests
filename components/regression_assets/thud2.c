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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <string.h>
#include "./test.h"
#include <sys/types.h>
#include <sys/wait.h>

/* These are used as strings so that strcmp can be used as a delay loop */
char *s1, *s2;

/* 20000 is fine on my 333MHz Celeron. Adjust so that system time is not
   excessive */
#define DELAY 200000
#define TEST_SECONDS 60

static unsigned long long get_nsecs(void)
{
	struct timespec myts;
	clock_gettime(CLOCK_REALTIME, &myts);
	return (myts.tv_sec * 1000000000ULL + myts.tv_nsec );
}


void busy_wait(long sec, long usec)
{
	struct timeval tv;
	long long end_usec;
	gettimeofday(&tv,0);
	end_usec=(long long)(sec+tv.tv_sec)*1000000 + tv.tv_usec+usec;
	while (((long long)tv.tv_sec*1000000 + tv.tv_usec) < end_usec)
	{
		gettimeofday(&tv,0);
		strcmp(s1,s2); /* yuck */
	}
}

#define PID_MAX 64

int main_thud2(int argc, char**argv)
{
 	struct timespec st={0,250000000};
	int n=DELAY;
	int parent=1;
	unsigned long long time;

	n = 10;

	s1=malloc(n);
	s2=malloc(n);
	memset(s1,33,n);
	memset(s2,33,n);
	s1[n-1]=0;
	s2[n-1]=0;

	fprintf(stderr,"starting %d children\n",n);
	for (; n>0; n--)
		if (test_children_fork() == 0) { sched_yield(); parent=0; break; }
	

	time = get_nsecs();
	while (get_nsecs() - time <  TEST_SECONDS * 1000000000ULL)
	{
		for (n=0; n<40; n++) nanosleep(&st, 0);
		if (parent) printf("running...");
		if (parent) fflush(stdout);
		busy_wait(6,0);
		if (parent) printf("done\n");
	}
	return test_children_gc(false);
}

