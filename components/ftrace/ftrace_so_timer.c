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

/* ftrace_so_task_timer - timer that moved */

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

#define FTRACE_SO_TIMER 0
static const char * ftrace_so_number[] =
{"timer", NULL};


#define TIMER_MAX 2048

struct ftrace_so_timer
{
	struct
	{
		double time;
		long long add;
		int cpu;
	} timers[TIMER_MAX];
	int nb;

	int not_started;
	int started;
	int moved;
	int canceled;
	int expires;
};


static void timer_add(struct ftrace_so_timer * task, int cpu, double time, long long add)
{
	int i;
	int empty = task->nb;
	task->started++;
	for (i = 0; i < task->nb; i++)
	{
		if (task->timers[i].cpu == -1)
			empty = i;
		if (task->timers[i].add == add)
			break;
	}
	if (task->nb == i)
	{
		i = empty;
		if (i == TIMER_MAX)
		{
			printf("Reach max number of timer\n");
			return;
		}
		if (task->nb == i)
			task->nb++;
		task->timers[i].add = add;
	}
	task->timers[i].cpu = cpu;
	task->timers[i].time = time;
}

static void timer_del(struct ftrace_so_timer * task, int cpu, double time, long long add)
{
	int i;
	task->canceled++;
	for (i = 0; i < task->nb; i++)
		if (task->timers[i].add == add)
		{
			task->timers[i].cpu = -1;
			return;
		}
}

static void timer_expire(struct ftrace_so_timer * task, int cpu, double time, long long add)
{
	int i;
	task->expires++;
	for (i = 0; i < task->nb; i++)
		if (task->timers[i].add == add)
			break;
	if (i == task->nb)
	{
		task->not_started++;
		return;
	}
	if ((task->timers[i].cpu != -1) && (task->timers[i].cpu != cpu))
	{
		task->moved++;
		printf("%llx started on cpu %d at %lf and expire at %lf on cpu %d\n", 
				add,
				task->timers[i].cpu,
				task->timers[i].time,
				time,
				cpu);
	}
	task->timers[i].cpu = -1;
}



static void timer_start(void * data,int cpu, double time, long long * number, const char **str)
{
	struct ftrace_so_timer * task =
		(struct ftrace_so_timer *)data;
	long long timer = number[FTRACE_SO_TIMER];
	timer_add(task, cpu, time, timer);
}

static void timer_expire_entry(void * data,int cpu, double time, long long * number, const char **str)
{
	struct ftrace_so_timer * task =
		(struct ftrace_so_timer *)data;
	long long timer = number[FTRACE_SO_TIMER];
	timer_expire(task, cpu, time, timer);
}

static void timer_cancel(void * data,int cpu, double time, long long * number, const char **str)
{
	struct ftrace_so_timer * task =
		(struct ftrace_so_timer *)data;
	long long timer = number[FTRACE_SO_TIMER];
	timer_del(task, cpu, time, timer);
}

static const char * ftrace_dl_info(void)
{
	return "This libraries does statistics about timer and look at timer that were added on one cpu and ended on another one.";
}

static void * ftrace_dl_init(void)
{
	struct ftrace_so_timer * task =
		(struct ftrace_so_timer *)
			malloc(sizeof(struct ftrace_so_timer));
	if (task == NULL)
		return NULL;
	task->nb = 0;
	task->not_started = 0;
	task->started = 0;
	task->moved = 0;
	task->canceled = 0;
	task->expires = 0;
	ftrace_decode_register("timer_start", timer_start, task, ftrace_so_number, NULL);
	ftrace_decode_register("timer_expire_entry", timer_expire_entry, task, ftrace_so_number, NULL);
	ftrace_decode_register("timer_cancel", timer_cancel, task, ftrace_so_number, NULL);
	return task;
}

static bool ftrace_dl_cleanup(void * data)
{
	struct ftrace_so_timer * task =
		(struct ftrace_so_timer *)
			data;
	if (task == NULL)
		return false;
	printf("#Started %d moved %d canceled %d expires %d\n",
		task->started,
		task->moved,
		task->canceled,
		task->expires);
	if (task->moved > 0)
	{
		free(data);
		return true;
	}
	return false;
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

