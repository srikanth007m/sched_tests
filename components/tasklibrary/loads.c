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

/* loads - busy loop and timer controller for load generator */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sched.h>
#include <sys/resource.h>
#include "timerload.h"

volatile int busy_loops = 100000000;
volatile int result_counter=0;
volatile int iterations_performed=0;
volatile int local_index = 0;
volatile int local_time = 0;
volatile int finish = 0;
volatile int timer_expiries = 0;
volatile int local_priority = 0;

/* time waster routine selects a never-ending point in
 * the mandelbrot set and computes the escape point
 * (which doesn't exist) until we have executed max_iteration
 * loops
 */
void waste_time(int max_iteration)
{
	double x=0,y=0, xtemp=0, xsquared=0,ysquared=0;
	double x0=0.0f,y0=0.0f;
	int iteration=0;

	while ( (xsquared + ysquared) < 4 && iteration <= max_iteration )
	{
		xsquared = x*x;
		ysquared = y*y;
		xtemp = xsquared - ysquared + x0;
		y = 2*x*y + y0;
		x = xtemp;
		iterations_performed = iteration++;
		__asm__ __volatile__ ("" ::: "memory");
	}
}

void catch_alarm (int sig)
{
	timer_expiries++;
	if(local_index < loadseq_to_run && local_time >= raw_loadseq_array[local_index+1].time)
		local_index++;
	busy_loops = raw_loadseq_array[local_index].loops;

	if(local_priority != raw_loadseq_array[local_index].priority)
	{
		if(!setpriority(PRIO_PROCESS, 0, raw_loadseq_array[local_index].priority))
			local_priority = raw_loadseq_array[local_index].priority;
	}

	if(busy_loops < 0)
	{
		finish = 1;
		iterations_performed = busy_loops;
	}
	waste_time(busy_loops);
	if(iterations_performed != busy_loops)
		printf("WARNING: Didn't perform all %d busy_loop iterations, did %d\n", busy_loops, iterations_performed);
	local_time++;
}

int loadseq_to_run=0;

void run_loadseq(void)
{
	printf("Generated load contains %d time points.\n", loadseq_to_run);
	/* debug print */

	{
		int i;
		for(i=0;i<loadseq_to_run; i++)
			printf("%d : %d %d\n",raw_loadseq_array[i].time, raw_loadseq_array[i].loops, raw_loadseq_array[i].priority);
	}

	local_index = 0;
	local_time  = 0;
	timer_expiries = 0;
	local_priority = getpriority(PRIO_PROCESS, 0);

	/* Establish a handler for SIGALRM signals. */
	if(signal(SIGALRM, catch_alarm) == SIG_ERR)
		return;
	/* Set an alarm to go off in a little while. */
	ualarm(MICROSECONDS_PER_LOOP,MICROSECONDS_PER_LOOP);

	while(!finish){
		pause();
	};

	printf("Timer expired %d times\n", timer_expiries);
	/* cancel alarm */
	ualarm(0,0);

	return;
}
