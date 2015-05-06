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

/* ftrace_so_task_hmp_migration - give migration time */

#include "ftrace_dl.h"
#include "ftrace_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 * Notice: all functions and variables must be static
 */

#define CPU_MAX 32
#define HISTOGRAM_MAX 128
#define STEP_PER_S 2000

#define FTRACE_SO_PREV_PID 0
static const char * ftrace_so_number[] =
{"prev_pid", NULL};


struct ftrace_so_timeslice_histogram
{
	int max_cpu;
	double time[CPU_MAX];
	int nb[CPU_MAX][HISTOGRAM_MAX];
	double sum[CPU_MAX][HISTOGRAM_MAX];
	
};

static void sched_switch(void * data,int cpu, double time, long long * number, const char **str)
{
	struct ftrace_so_timeslice_histogram * task =
		(struct ftrace_so_timeslice_histogram *)data;
	int pid = number[FTRACE_SO_PREV_PID];

	if ((pid != 0) && (task->time[cpu] != -HUGE_VAL))
	{
		int nb = (time - task->time[cpu]) * STEP_PER_S;
		if (nb >= HISTOGRAM_MAX)
			nb = HISTOGRAM_MAX - 1;
		task->nb[cpu][nb]++;
		task->sum[cpu][nb] += time - task->time[cpu];
	}
	task->time[cpu] = time;
	if (task->max_cpu <= cpu)
		task->max_cpu = cpu + 1;
}

static const char * ftrace_dl_info(void)
{
	return "This libraries does statistics about timeslice on each cpu. Always success.";
}

static void * ftrace_dl_init(void)
{
	int cpu,j;
	struct ftrace_so_timeslice_histogram * task =
		(struct ftrace_so_timeslice_histogram *)
			malloc(sizeof(struct ftrace_so_timeslice_histogram));
	if (task == NULL)
		return NULL;
	task->max_cpu = 0;
	for (cpu = 0; cpu < CPU_MAX; cpu++)
	{
		task->time[cpu] = -HUGE_VAL;
		for (j = 0; j < HISTOGRAM_MAX; j++)
		{
			task->nb[cpu][j] = 0;
			task->sum[cpu][j] = 0;
		}
	}
	ftrace_decode_register("sched_switch", sched_switch, task, ftrace_so_number, NULL);
	return task;
}

static bool ftrace_dl_cleanup(void * data)
{
	int j;
	int cpu;
	struct ftrace_so_timeslice_histogram * task =
		(struct ftrace_so_timeslice_histogram *)
			data;
	if (task == NULL)
		return false;

	printf("#timeslice_used_s ");
	for (cpu = 0; cpu < task->max_cpu; cpu++)
		printf("cpu%d_nb cpu%d_sum ", cpu, cpu);
	printf("allcpu_nb allcpu_sum\n");
	
	for (j = 0; j < HISTOGRAM_MAX; j++)
	{
		int nb_sum = 0;
		double sum_sum = 0;
		printf("%lf ", ((double)j)/STEP_PER_S);
		for (cpu = 0; cpu < task->max_cpu; cpu++)
		{
			printf("%d %lf ", task->nb[cpu][j], task->sum[cpu][j]);
			nb_sum += task->nb[cpu][j];
			sum_sum += task->sum[cpu][j];
		}
		printf("%d %lf\n", nb_sum, sum_sum);
	}
	free(data);
	return true;
}

/*
 * These macro are required
 * but ftrace_dl_init, ftrace_dl_info and ftrace_dl_cleanup
 * can have a different name, the following macro give
 * their names for export in .so or in static
 */ 
FTRACE_SO_INIT(ftrace_dl_init)
FTRACE_SO_INFO(ftrace_dl_info)
FTRACE_SO_CLEANUP(ftrace_dl_cleanup)

