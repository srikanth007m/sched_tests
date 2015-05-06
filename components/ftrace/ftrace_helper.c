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

/* ftrace_helper - helper for common ftrace analysis library */

#include "ftrace_helper.h"
#include "ftrace_decode.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define FTRACE_HELPER_CPU_MAX 16
#define FTRACE_HELPER_TASK_MAX 65536

static struct
{
	struct
	{
		int cpu;
		double sched_in;
		double sched_out;
		bool running;
		bool waiting;
		double last_running;
		double last_waiting;
	} task[FTRACE_HELPER_TASK_MAX];
	struct 
	{
		int pid;
		long long frequency;
	} cpu[FTRACE_HELPER_CPU_MAX];
} ftrace_helper_data;

#define FTRACE_SO_NEXT_PID 0
#define FTRACE_SO_PREV_PID 1
static const char * ftrace_so_number_sched_switch[] =
{"next_pid", "prev_pid", NULL};

#define FTRACE_SO_PID 0
static const char * ftrace_so_number_sched_wakeup[] =
{"pid", NULL};

#define FTRACE_SO_STATE 0
#define FTRACE_SO_CPU_ID 1
static const char * ftrace_so_number_power_frequency[] =
{"state", "cpu_id", NULL};

int ftrace_helper_getenv_array(const char * name, double * values, int max)
{
	char * data = getenv(name);
	char * next;
	int i;
	if (data == NULL)
	{
		if (strcmp(name, "CPU_FAST") == 0)
		{
			for (i = 0; (i < 2) && (i < max); i++)
				values[i] = i;
			return i;
		}
		if (strcmp(name, "CPU_SLOW") == 0)
		{
			for (i = 0; (i < 3) && (i < max); i++)
				values[i] = i + 2;
			return i;
		}
		return -1;
	}
	i = 0;
	while ((*data != 0) && (i < max))
	{
		values[i] =  strtod(data, &next);
		if (next == data)
			return i;
		data = next;
		i++;
		if (*data != ',')
			return i;
		data++;
	}
	return i;	
}


int ftrace_helper_getenv_array_int(const char * name, int * values, int max)
{
	char * data = getenv(name);
	char * next;
	double temp;
	int i;
	if (data == NULL)
	{
		if (strcmp(name, "CPU_FAST") == 0)
		{
			for (i = 0; (i < 2) && (i < max); i++)
				values[i] = i;
			return i;
		}
		if (strcmp(name, "CPU_SLOW") == 0)
		{
			for (i = 0; (i < 3) && (i < max); i++)
				values[i] = i + 2;
			return i;
		}
		return -1;
	}
	i = 0;
	while ((*data != 0) && (i < max))
	{
		temp =  strtod(data, &next);
		if (next == data)
			return i;
		if ((temp > 2LL*1024LL*1024LL) || (temp < - 2LL*1024LL*1024LL))
			return i;
		values[i] = (int)temp;
		data = next;
		i++;
		if (*data != ',')
			return i;
		data++;
	}
	return i;
}

bool ftrace_helper_cpu_info(int cpu, int * pid, long long * frequency)
{
	if ((cpu < 0) || (cpu >= FTRACE_HELPER_CPU_MAX))
		return false;
	if (ftrace_helper_data.cpu[cpu].pid < 0)
		return false;
	if (ftrace_helper_data.cpu[cpu].frequency < 0)
		return false;
	*pid = ftrace_helper_data.cpu[cpu].pid;
	*frequency = ftrace_helper_data.cpu[cpu].frequency;
	return true;
}

bool ftrace_helper_task_info(int pid, int * cpu, double * sched_in, double * sched_out, bool * waiting, bool * running)
{
	if ((pid < 0) || (pid >= FTRACE_HELPER_TASK_MAX))
		return false;
	*cpu = ftrace_helper_data.task[pid].cpu;
	*sched_in = ftrace_helper_data.task[pid].sched_in;
	*sched_out = ftrace_helper_data.task[pid].sched_out;
	*waiting = ftrace_helper_data.task[pid].waiting;
	*running = ftrace_helper_data.task[pid].running;
	return true;
}

bool ftrace_helper_task_waiting(int pid, double * last_running, double * last_waiting)
{
	if ((pid < 0) || (pid >= FTRACE_HELPER_TASK_MAX))
		return false;
	*last_waiting = ftrace_helper_data.task[pid].last_waiting;
	*last_running = ftrace_helper_data.task[pid].last_running;
	return true;
}

static void ftrace_helper_sched_wakeup(void * data,int cpu, double time, long long * number, const char ** str)
{
	int pid = number[FTRACE_SO_PID];
	if ((pid < 0) || (pid >= FTRACE_HELPER_TASK_MAX))
		return;
	ftrace_helper_data.task[pid].waiting = true;
	ftrace_helper_data.task[pid].last_waiting = time - ftrace_helper_data.task[pid].sched_out;
}

static void ftrace_helper_power_frequency(void * data,int cpu, double time, long long * number, const char ** str)
{
	int state = number[FTRACE_SO_STATE];
	int cpu_id = number[FTRACE_SO_CPU_ID];
	ftrace_helper_data.cpu[cpu_id].frequency = state * 1000; /* convert to HZ */
}

static void ftrace_helper_sched_switch(void * data,int cpu, double time, long long * number, const char ** str)
{
	int prev_pid = number[FTRACE_SO_PREV_PID];
	int next_pid = number[FTRACE_SO_NEXT_PID];
	ftrace_helper_data.cpu[cpu].pid = next_pid;
	if ((prev_pid >= 0) && (prev_pid < FTRACE_HELPER_TASK_MAX))
	{
		ftrace_helper_data.task[prev_pid].sched_out = time;
		ftrace_helper_data.task[prev_pid].running = false;
		if (ftrace_helper_data.task[next_pid].waiting)
		{
			ftrace_helper_data.task[prev_pid].last_waiting = false;
			ftrace_helper_data.task[prev_pid].last_running = 0;
		}
		ftrace_helper_data.task[prev_pid].last_running += 
				time - ftrace_helper_data.task[prev_pid].sched_in;

	}
	if ((next_pid >= 0) && (next_pid < FTRACE_HELPER_TASK_MAX))
	{
		ftrace_helper_data.task[next_pid].sched_in = time;
		ftrace_helper_data.task[next_pid].running = true;
	}
}

void ftrace_helper_init(void)
{
	int cpu, task;
	for (cpu = 0; cpu < FTRACE_HELPER_CPU_MAX; cpu++)
	{
		ftrace_helper_data.cpu[cpu].pid = -1;
		ftrace_helper_data.cpu[cpu].frequency = -1;
	}
	for (task = 0; task < FTRACE_HELPER_CPU_MAX; task++)
	{
		ftrace_helper_data.task[task].cpu = -1;
		ftrace_helper_data.task[task].sched_in = -HUGE_VAL;
		ftrace_helper_data.task[task].sched_out = -HUGE_VAL;
		ftrace_helper_data.task[task].running = false;
		ftrace_helper_data.task[task].waiting = true;
	}

	/*
		The first register handler is the last called, since ftrace_helper
		handler is registered at the start, it will always be called after
		any other handler.
	 */
	ftrace_decode_register("sched_switch", ftrace_helper_sched_switch, NULL, ftrace_so_number_sched_switch, NULL);
	ftrace_decode_register("sched_wakeup", ftrace_helper_sched_wakeup, NULL, ftrace_so_number_sched_wakeup, NULL);
	ftrace_decode_register("power_frequency", ftrace_helper_power_frequency, NULL, ftrace_so_number_power_frequency, NULL);
}

void ftrace_helper_cleanup(void)
{
	ftrace_decode_register("power_frequency", ftrace_helper_power_frequency, NULL, ftrace_so_number_power_frequency, NULL);
	ftrace_decode_register("sched_wakeup", ftrace_helper_sched_wakeup, NULL, ftrace_so_number_sched_wakeup, NULL);
	ftrace_decode_unregister("sched_switch", ftrace_helper_sched_switch);
}

#include <stdio.h> /* not needed by previous function */

struct histogram
{
	double min;
	double step;
	int step_nb;
	int step_max;
	int index_max;
	int index_nb;
	double * restrict time;
	double * restrict sumtime;
	double * restrict sumvalue;
};

struct histogram * histogram_create(int index_max, int step_max, double min, double step)
{
	int i, j;
	struct histogram * hist = 
		(struct histogram *)malloc(sizeof(struct histogram)
					+ sizeof(double) * index_max * step_max
					+ sizeof(double) * index_max
					+ sizeof(double) * index_max);
	if (hist == NULL)
		return NULL;
	hist->time = (double *)(hist + 1);
	hist->sumtime = hist->time + index_max * step_max;
	hist->sumvalue = hist->sumtime + index_max;
	hist->min = min;
	hist->step = step;
	hist->step_nb = 0;
	hist->step_max = step_max;
	hist->index_nb = 0;
	hist->index_max = index_max;
	for (i = 0; i < index_max; i++)
		for (j = 0; j < step_max; j++)
			hist->time[i * step_max + j] = 0;

	for (i = 0; i < index_max; i++)
	{
		hist->sumvalue[i] = 0;
		hist->sumtime[i] = 0;
	}
	return hist;
}

void histogram_destroy(struct histogram * hist)
{
	if (hist != NULL)
		free(hist);
}

void histogram_dump(struct histogram * hist, const char * filename)
{
	int index;
	int step;
	FILE * fd;
	if (hist == NULL)
		return;
	if (strcmp(filename, "") == 0)
		fd = stdout;
	else
		fd = fopen(filename, "w");
	if (fd == NULL)
		return;
	for (step = 0; step < hist->step_nb; step++)
	{
		fprintf(fd, "%lf", (step + hist->min) * hist->step);
		for (index = 0; index < hist->index_nb; index++)
			fprintf(fd, " %lf", hist->time[index * hist->step_max + step]/hist->sumtime[index]);
		fprintf(fd, "\n");
	}
	if (fd != stdout)
		fclose(fd);
}

void histogram_info(struct histogram * hist, int index, double * time, double * avg)
{
	if (hist == NULL)
	{
		*time = 0;
		*avg = 0;
		return;
	}
	if ((index < 0) || (index >= hist->index_nb))
	{
		*time = 0;
		*avg = 0;
		return;
	}
	*avg = hist->sumvalue[index]/hist->sumtime[index];
	*time =	hist->sumtime[index];
}

void histogram_add(struct histogram * restrict hist, int index, double time, double value)
{
	int nb;
	if (hist == NULL)
		return;
	if ((index < 0) || (index >= hist->index_max))
		return;
	if (time <= 0)
		return;
	if (value != value) /* Not a number == NaN */
		return;
	value = (value - hist->min) / hist->step;
	if (value < 0)
		value = 0;
	else if (value >= hist->step_max)
		value = hist->step_max - 1;
	nb = (int)value;
	if (nb >= hist->step_nb)
		hist->step_nb = nb + 1;
	if (index >= hist->index_nb)
		hist->index_nb = index + 1;
	hist->time[index * hist->step_max + nb] += time;
	hist->sumtime[index] += time;
	hist->sumvalue[index] += time * value;
}

