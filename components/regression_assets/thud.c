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
/*
 * 3 Aug 2012 Changed by Olivier Cozette olivier.cozette@arm.com
 *    - Run at most for 60s
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <string.h>
#include "./test.h"

/* These are used as strings so that strcmp can be used as a delay loop */
static char *s1, *s2;

/* 20000 is fine on my 333MHz Celeron. Adjust so that system time is not
   excessive */
#define DELAY 200000

static void busy_wait(long sec, long usec)
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

int main_thud(int argc, char**argv)
{
	struct timespec st={10,50000000};
	int n=DELAY;
	int parent=1;
	int i;

	s1=malloc(n);
	s2=malloc(n);
	memset(s1,33,n);
	memset(s2,33,n);
	s1[n-1]=0;
	s2[n-1]=0;

	n=20;
	fprintf(stderr,"starting %d children\n",n);
	for (; n>0; n--)
		if (test_children_fork()==0) { sched_yield(); parent=0; break; }
	
	for (i = 0; i < 3; i++)
	{
		/* FIXME: never fail */
		nanosleep(&st, 0);
		if (parent) printf("running...");
		if (parent) fflush(stdout);
		busy_wait(6,0);
		if (parent) printf("done\n");
	}
	return test_children_gc(false);
}

