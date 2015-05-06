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

/* ftrace_so_biglittleswitch - give a matrix of selected process over time */

/*
 * Algorithm:
 *  task->interval_start is a list of interval where
 *  1) all START_BIG with priority START_BIG_PRIORITY are on CPU_FAST cpus
 *  2) all START_LITTLE with priority START_LITTLE_PRIORITY are on CPU_SLOW cpus
 *  3) no interval lasting longer than EXPECTED_CHANGE_TIME_MS_MAX
 *  4) no interval lasting less than EXPECTED_CHANGE_TIME_MS_MIN
 *
 *  task->interval_stop is a list of interval where
 *  1) all END_BIG with priority END_BIG_PRIORITY are on CPU_FAST cpus
 *  2) all END_LITTLE with priority END_LITTLE_PRIORITY are on CPU_SLOW cpus
 *  3) no interval lasting less than EXPECTED_TIME_IN_END_STATE_MS
 *
 * To be successfull we must have one the following two conditions:
 *    1) Have at least one interval INT1 in interval_start and INT2 in interval_stop where
 *       1) INT1 end before INT2 start
 *       2) Intersect of INT1 and INT2 is empty
 *    2) Have at least one interval INT1 in interval_start and INT2 in interval_stop where
 *       2) INT1 and INT2 are not emptpy
 *       3) START_BIG = END_BIG
 *       4) START_BIG_PRIORITY = END_BIG_PRIORITY
 *       5) START_LITTLE = END_LITTLE
 *       6) START_LITTLE_PRIORITY = END_LITTLE_PRIORITY
 */

#include "ftrace_dl.h"
#include "ftrace_decode.h"
#include "ftrace_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 * Notice: all functions and variables must be static
 */

#define CPU_MAX 32
#define PID_MAX 128

#define FTRACE_SO_NEXT_PID 0
#define FTRACE_SO_NEXT_PRIO 1
#define FTRACE_SO_PREV_PID 2
#define FTRACE_SO_PREV_PRIO 3

static const char * ftrace_so_number[] =
{"next_pid", "next_prio", "prev_pid", "prev_prio", NULL};

#define FTRACE_SO_PID 0
static const char * ftrace_so_number_sched_wakeup[] =
{"pid", NULL};

struct ftrace_so_interval
{
	const char * name;
	double min;
	double max;
	double start;
	double stop;
	struct ftrace_so_interval * next;
};

struct ftrace_so_biglittleswitch
{
	struct ftrace_so_interval interval_start;
	struct ftrace_so_interval interval_end;
	double last_time;
	int wakeup_slow_nb;
	bool cpu_fast[CPU_MAX];
	int monitored_nb;
	struct
	{
		int pid;
		int  start_prio;
		int  end_prio;
		bool start_fast;
		bool end_fast;
		bool current_fast;
		int current_prio;
		int last_wakeup_cpu;
		double last_wakeup;
	} monitored[PID_MAX];

	/* index in monitored_pid is monitored_nb > 0
	 * otherwise it's a pid */
	double expected_change_time_ms_min;
	double expected_change_time_ms_max;
	double expected_time_in_end_state_ms;
};

static void ftrace_so_interval_init(struct ftrace_so_interval * interval, const char * name, double minimum, double maximum)
{
	interval->name = name;
	interval->min = minimum;
	interval->max = maximum;
	interval->start = HUGE_VAL;
	interval->stop = -HUGE_VAL;
	interval->next = NULL;
}

static void ftrace_so_interval_grow(struct ftrace_so_interval * interval, double time)
{
	if (interval->start == HUGE_VAL)
		interval->start = time;
	interval->stop = time;
}

static void ftrace_so_interval_end(struct ftrace_so_interval * interval, double time)
{
	struct ftrace_so_interval * temp;
	if (interval->start == HUGE_VAL)
		return;
	if ((interval->stop - interval->start < interval->min)
		||  (interval->stop - interval->start > interval->max))
	{
		/* discard this interval */
		if (interval->stop - interval->start < interval->min)
			printf("# %s discard interval [%lf, %lf] size %lf < %lf\n",
			interval->name, interval->start, interval->stop,
			interval->stop - interval->start, interval->min);
		else
			printf("# %s discard interval [%lf, %lf]  size %lf > %lf\n",
			interval->name, interval->start, interval->stop,
			interval->stop - interval->start, interval->max);
		interval->start = HUGE_VAL;
		interval->stop = -HUGE_VAL;
		return;
	}
	temp = (struct ftrace_so_interval *)
		malloc(sizeof(struct ftrace_so_interval));
	if (temp != NULL)
	{
		*temp = *interval;
		interval->next = temp;
	}
	/* discard this interval */
	interval->start = HUGE_VAL;
	interval->stop = -HUGE_VAL;
}

static void ftrace_so_interval_cleanup(struct ftrace_so_interval * interval)
{
	while (interval->next != NULL)
	{
		struct ftrace_so_interval * temp = interval->next;
		interval->next = interval->next->next;
		free(temp);
	}
}

/*
 * find biggest interval in interval1 and interval2 this the following 
 * condition applied in the following order
 * 1) interval1 happens before interval2
 * 2) size(interval2) the biggest while 1) is met
 * 3) size(interval1) the biggest while 1) and 2) are met
 */
static bool ftrace_so_interval_find_nocross(struct ftrace_so_interval * interval1, struct ftrace_so_interval * interval2,
		double * restrict start1, double * restrict stop1, double * restrict start2, double * restrict stop2)
{
	struct ftrace_so_interval * int1;
	struct ftrace_so_interval * int2;
	*start1 = HUGE_VAL;
	*start2 = HUGE_VAL;
	*stop1 = -HUGE_VAL;
	*stop2 = -HUGE_VAL;
	for (int1 = interval1->next; int1 != NULL; int1 = int1->next)
		for (int2 = interval2->next; int2 != NULL; int2 = int2->next)
		{
			if (int1->stop > int2->start)
				continue;
			if ((*start2 != HUGE_VAL) && (*stop2 - *start2 > int2->stop - int2->start))
				continue;
			if ((*start1 != HUGE_VAL) && (*stop1 - *start1 > int1->stop - int1->start))
				continue;
			*start1 = int1->start;
			*stop1 = int1->stop;
			*start2 = int2->start;
			*stop2 = int2->stop;
		}
	if (*start1 != HUGE_VAL)
		return true;
	return false;
}

/*
 * find the biggest intervals in interval1 and interval2
 * One or both intervals could be [+Inf, -Inf] i.e. empty.
 */
static void ftrace_so_interval_find(struct ftrace_so_interval * interval1, struct ftrace_so_interval * interval2,
		double * restrict start1, double * restrict stop1, double * restrict start2, double * restrict stop2)
{
	struct ftrace_so_interval * int1;
	struct ftrace_so_interval * int2;
	*start1 = HUGE_VAL;
	*start2 = HUGE_VAL;
	*stop1 = -HUGE_VAL;
	*stop2 = -HUGE_VAL;
	for (int1 = interval1->next; int1 != NULL; int1 = int1->next)
	{
		if ((*start1 != HUGE_VAL) && (*stop1 - *start1 > int1->stop - int1->start))
			continue;
		*start1 = int1->start;
		*stop1 = int1->stop;
	}
	for (int2 = interval2->next; int2 != NULL; int2 = int2->next)
	{
		if ((*start2 != HUGE_VAL) && (*stop2 - *start2 > int2->stop - int2->start))
			continue;
		*start2 = int2->start;
		*stop2 = int2->stop;
	}
}

static void ftrace_wake_pids(struct ftrace_so_biglittleswitch * task, int pid, int cpu, double time)
{
	int i;
	for (i = 0; i < task->monitored_nb; i++)
        {
                if (task->monitored[i].pid == pid)
                        break;
        }
        if (i == task->monitored_nb)
                return;
	task->monitored[i].last_wakeup = time;
	task->monitored[i].last_wakeup_cpu = cpu;
}

static void ftrace_wake_check_pids(struct ftrace_so_biglittleswitch * task, int pid, int cpu, int prev_pid, double time)
{
	int i;
	if (prev_pid != 0)
		return;
        for (i = 0; i < task->monitored_nb; i++)
        {
                if (task->monitored[i].pid == pid)
                        break;
        }
        if (i == task->monitored_nb)
                return;
	if (time - task->monitored[i].last_wakeup > 0.003)
	{
		printf("# %lf pid %d wake on cpu %d delayed by %3.0lfms on cpu %d prev_pid=%d\n",
			time, pid, task->monitored[i].last_wakeup_cpu,
			(time - task->monitored[i].last_wakeup)*1000.0, cpu, prev_pid);
		task->wakeup_slow_nb++;
	}
}

static void ftrace_update_pids(struct ftrace_so_biglittleswitch * task, int pid, int cpu, int prio, double time)
{
	int i;
	for (i = 0; i < task->monitored_nb; i++)
		if (task->monitored[i].pid == pid)
			break;
	if (i == task->monitored_nb)
		return;
	task->monitored[i].last_wakeup = HUGE_VAL;

	/* in any case, a change of priority close end the current interval */
	if (task->monitored[i].current_prio != prio)
	{
		ftrace_so_interval_end(&task->interval_start, time);
		ftrace_so_interval_end(&task->interval_end, time);
	}

	task->monitored[i].current_prio = prio;
	task->monitored[i].current_fast = task->cpu_fast[cpu];

	for (i = 0; i < task->monitored_nb; i++)
	{
		/*if ((task->monitored[i].start_prio != task->monitored[i].current_prio)
			&& (task->monitored[i].start_prio != -1))
			break;*/
		if (task->monitored[i].start_fast != task->monitored[i].current_fast)
			break;
	}
	if (i < task->monitored_nb)
		ftrace_so_interval_end(&task->interval_start, time);
	else
		ftrace_so_interval_grow(&task->interval_start, time);

	for (i = 0; i < task->monitored_nb; i++)
	{
		/*if ((task->monitored[i].end_prio != task->monitored[i].current_prio)
			&& (task->monitored[i].end_prio != -1))
			break;*/
		if (task->monitored[i].end_fast != task->monitored[i].current_fast)
			break;
	}
	if (i < task->monitored_nb)
		ftrace_so_interval_end(&task->interval_end, time);
	else
		ftrace_so_interval_grow(&task->interval_end, time);
}

static bool ftrace_update_expected_start_equal_expected_end(struct ftrace_so_biglittleswitch * task)
{
	int i;
	for (i = 0; i < task->monitored_nb; i++)
	{
		/*if (task->monitored[i].start_prio != task->monitored[i].end_prio)
			return false;*/
		if (task->monitored[i].start_fast != task->monitored[i].end_fast)
			return false;
	}
	return true;
}

static void sched_switch(void * data,int cpu, double time, long long * number, const char **str)
{
	int prev_pid = number[FTRACE_SO_PREV_PID];
	int next_pid = number[FTRACE_SO_NEXT_PID];
	int prev_prio = number[FTRACE_SO_PREV_PRIO];
	int next_prio = number[FTRACE_SO_NEXT_PRIO];
	struct ftrace_so_biglittleswitch * task =
		(struct ftrace_so_biglittleswitch *)data;
	ftrace_wake_check_pids(task, next_pid, cpu, prev_pid, time);
	ftrace_update_pids(task, prev_pid, cpu, prev_prio, time);
	ftrace_update_pids(task, next_pid, cpu, next_prio, time);

	task->last_time = time;
}

static void ftrace_helper_sched_wakeup(void * data,int cpu, double time, long long * number, const char ** str)
{
	struct ftrace_so_biglittleswitch * task =
                (struct ftrace_so_biglittleswitch *)data;
        int pid = number[FTRACE_SO_PID];
	ftrace_wake_pids(task, pid, cpu, time);
}

static const char * ftrace_dl_info(void)
{
	return "This library check if a configuration moved to another one in a defined amount of time\n"
		"Conf start\n"
		"\tSTART_BIG=pid0,pid1,... START_BIG_PRIORITY=pid0_prio,pid1_prio,...\n"
		"\tSTART_LTTILE=pid100,pid101,... START_LITTLE_PRIORITY=pid100_prio,pid101_prio,...\n"
		"Conf end\n"
		"\tEND_BIG=pid0,pid1,... END_BIG_PRIORITY=pid0_prio,pid1_prio,...\n"
		"\tEND_LTTILE=pid100,pid101,... END_LITTLE_PRIORITY=pid100_prio,pid101_prio,...\n"
		"Time to move from first to milliseconds:\n"
		"From EXPECTED_CHANGE_TIME_MS_MIN to EXPECTED_CHANGE_TIME_MS_MAX"
		"Minimum time in end state in milliseconds:\n"
		"From EXPECTED_TIME_IN_END_STATE_MS";
}

static void ftrace_init_add_pids(struct ftrace_so_biglittleswitch * task, int pid, bool start, bool fast, int prio)
{
	int i;
	for (i = 0; i < task->monitored_nb; i++)
	{
		if (task->monitored[i].pid == pid)
			break;
	}
	if (i == PID_MAX)
		return;
	if (i == task->monitored_nb)
	{
		task->monitored[i].pid = pid;
		task->monitored[i].start_fast = false;
		task->monitored[i].end_fast = false;
		task->monitored[i].current_fast = false;
		task->monitored[i].current_prio = prio;
		task->monitored[i].start_prio = prio;
		task->monitored[i].end_prio = prio;
		task->monitored[i].last_wakeup = HUGE_VAL;
		task->monitored_nb++;
	}
	if (start)
		task->monitored[i].start_fast = fast;
	else
		task->monitored[i].end_fast = fast;
}


static void ftrace_read_env(struct ftrace_so_biglittleswitch * task)
{
	int j, nb, nbprio;
	int temp[PID_MAX];
	int prio[PID_MAX];
	for (j = 0; j < CPU_MAX; j++)
		task->cpu_fast[j] = false;
	nb = ftrace_helper_getenv_array_int("CPU_FAST", &temp[0], CPU_MAX);
	for (j = 0; j < nb; j++)
	{
		if ((temp[j] < 0) || (temp[j] >= CPU_MAX))
			continue;
		task->cpu_fast[temp[j]] = true;
	}

	nb = ftrace_helper_getenv_array("EXPECTED_CHANGE_TIME_MS_MIN",
		&task->expected_change_time_ms_min, 1);
	if (nb < 1)
		task->expected_change_time_ms_min = -HUGE_VAL;

	nb = ftrace_helper_getenv_array("EXPECTED_CHANGE_TIME_MS_MAX",
		&task->expected_change_time_ms_max, 1);
	if (nb < 1)
		task->expected_change_time_ms_max = HUGE_VAL;
	nb = ftrace_helper_getenv_array("EXPECTED_TIME_IN_END_STATE_MS",
		&task->expected_time_in_end_state_ms, 1);
	if (nb < 1)
		task->expected_time_in_end_state_ms = 0;

	ftrace_so_interval_init(&task->interval_start, "START STATE",
				task->expected_change_time_ms_min/1000.0,
				task->expected_change_time_ms_max/1000.0);
	ftrace_so_interval_init(&task->interval_end, "END STATE",
				task->expected_time_in_end_state_ms/1000.0,
				HUGE_VAL);
	task->monitored_nb = 0;

	nb = ftrace_helper_getenv_array_int("START_LITTLE", &temp[0], PID_MAX);
	nbprio = ftrace_helper_getenv_array_int("START_LITTLE_PRIORITY", &prio[0], PID_MAX);
	if (nb > nbprio)
		nb = nbprio;
	for (j = 0; j < nb; j++)
		ftrace_init_add_pids(task, temp[j], true, false, prio[j]);

	nb = ftrace_helper_getenv_array_int("START_BIG", &temp[0], PID_MAX);
	nbprio = ftrace_helper_getenv_array_int("START_BIG_PRIORITY", &prio[0], PID_MAX);
	if (nb > nbprio)
		nb = nbprio;
	for (j = 0; j < nb; j++)
		ftrace_init_add_pids(task, temp[j], true, true, prio[j]);

	nb = ftrace_helper_getenv_array_int("END_LITTLE", &temp[0], PID_MAX);
	nbprio = ftrace_helper_getenv_array_int("END_BIG_PRIORITY", &prio[0], PID_MAX);
	if (nb > nbprio)
		nb = nbprio;
	for (j = 0; j < nb; j++)
		ftrace_init_add_pids(task, temp[j], false, false, prio[j]);

	nb = ftrace_helper_getenv_array_int("END_BIG", &temp[0], PID_MAX);
	nbprio = ftrace_helper_getenv_array_int("END_BIG_PRIORITY", &prio[0], PID_MAX);
	if (nb > nbprio)
		nb = nbprio;
	for (j = 0; j < nb; j++)
		ftrace_init_add_pids(task, temp[j], false, true, prio[j]);

	task->last_time = -HUGE_VAL;

	task->wakeup_slow_nb = 0;
	if (ftrace_helper_getenv_array_int("ERROR_ON_DELAY", &temp[0], 1) > 0)
		task->wakeup_slow_nb = 0;
	else
		task->wakeup_slow_nb = -10000; /* unilikely to reach 1 */
}

static void tracing_mark_write(void * data,int cpu, double time, long long * number, const char ** str)
{
	struct ftrace_so_biglittleswitch * task =
		(struct ftrace_so_biglittleswitch *)data;
	/* this is a test start marker, so discard everything done before */
	ftrace_read_env(task);
}

static void * ftrace_dl_init(void)
{
	struct ftrace_so_biglittleswitch * task =
		(struct ftrace_so_biglittleswitch *)
			malloc(sizeof(struct ftrace_so_biglittleswitch));
	if (task == NULL)
		return NULL;
	task->monitored_nb = 0;
	task->last_time = HUGE_VAL;
	ftrace_read_env(task);
	ftrace_decode_register("tracing_mark_write", tracing_mark_write, task, NULL, NULL);
	ftrace_decode_register("sched_switch", sched_switch, task, ftrace_so_number, NULL);
	ftrace_decode_register("sched_wakeup", ftrace_helper_sched_wakeup, task, ftrace_so_number_sched_wakeup, NULL);
	return task;
}

static bool ftrace_dl_cleanup(void * data)
{
	double start1;
	double stop1;
	double start2;
	double stop2;
	const char * message;
	struct ftrace_so_biglittleswitch * task =
		(struct ftrace_so_biglittleswitch *)
			data;
	if (task == NULL)
		return false;

	/* close current interval */
	ftrace_so_interval_end(&task->interval_start, task->last_time);
	ftrace_so_interval_end(&task->interval_end, task->last_time);

	do
	{
		message = NULL;
		if (ftrace_so_interval_find_nocross(&task->interval_start, &task->interval_end,
			&start1, &stop1, &start2, &stop2))
			break;

		ftrace_so_interval_find(&task->interval_start, &task->interval_end,
			&start1, &stop1, &start2, &stop2);

		message = "start and end state not found";
		if ((start1 == HUGE_VAL) && (start2 == HUGE_VAL))
			break;

		message = "start state not found";
		if (start1 == HUGE_VAL)
			break;

		message = "end state not found";
		if (start2 == HUGE_VAL)
			break;

		message = NULL;
		if (ftrace_update_expected_start_equal_expected_end(task))
			break; /* start and stop interval defined and start and end state equal, so it's fine */

		message = "start state cross end state";
	} while (0);

	printf("# Start state reach at %lf\n", start1);
	printf("# from start to end  %lf must be between %lf and %lf\n",
			stop1 - start1,
			task->expected_change_time_ms_min / 1000.0,
			task->expected_change_time_ms_max / 1000.0);
	printf("# End state reach at %lf\n", start2);
	printf("# End state lasted %lf must be greater than %lf\n",
			stop2 - start2,
			task->expected_time_in_end_state_ms / 1000.0);
	printf("# %s %s\n",
		(message == NULL)?"SUCCESS":"FAILED",
		(message == NULL)?"":message);

	if (message != NULL)
	{
		/* give more details on the failure */
		printf("# Start state big enought (> %lfs) found [%lf,%lf] = %lf s\n",
			task->expected_change_time_ms_min / 1000.,
			start1, stop1, stop1 - start1);
		printf("# Stop state big enought (> %lfs] found [%lf,%lf] = %lf s\n",
			task->expected_time_in_end_state_ms / 1000.,
			start2, stop2, stop2 - start2);
	}

	ftrace_so_interval_cleanup(&task->interval_start);
	ftrace_so_interval_cleanup(&task->interval_end);
	free(data);
	return (message == NULL);
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

