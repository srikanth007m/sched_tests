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

/* ftrace_so_process_matrix - give a matrix of selected process over time */

#include "ftrace_dl.h"
#include "ftrace_decode.h"
#include "ftrace_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/*
 * Notice: all functions and variables must be static
 */

#define CPU_MAX 32
#define HISTOGRAM_MAX 128
#define PID_MAX 128

#define FTRACE_SO_NEXT_PID 0
static const char * ftrace_so_number[] =
{"next_pid", NULL};

struct ftrace_so_process_matrix
{
	int max_cpu;
	int monitored_nb;
	int monitored_pid[PID_MAX];

	double ovrl_total;
	double cpus_total[CPU_MAX];
	double task_total[PID_MAX];
	double time_total[PID_MAX][CPU_MAX];
	double last_start[PID_MAX][CPU_MAX];

	int cpus_mask;
	double usage_min;
	double usage_max;
	double time_min;
	double time_max;
	bool residency_test_passed;
	bool time_test_passed;
	double usage_sum;
	double time_sum;

	/* index in monitored_pid is monitored_nb > 0
	 * otherwise it's a pid */
	int monitored_current[CPU_MAX];
	double start;
	double stop;
};

static void sched_switch(void * data,int cpu, double time, long long *number, const char **str)
{
	int i;
	struct ftrace_so_process_matrix * task =
		(struct ftrace_so_process_matrix *)data;
	int pid = number[FTRACE_SO_NEXT_PID];
	int tix;

	if (task->max_cpu <= cpu)
		task->max_cpu = cpu + 1;

	/* Find IDX of current switching task */
	for (i = 0; i < task->monitored_nb; i++)
		if (pid == task->monitored_pid[i])
			break;

	tix = task->monitored_current[cpu];

	/* Task exiting the CPU */
	if (tix != i && task->last_start[tix][cpu] != 0) {

		double delta = time - task->last_start[tix][cpu];
		task->ovrl_total           += delta;
		task->cpus_total[cpu]      += delta;
		task->task_total[tix]      += delta;
		task->time_total[tix][cpu] += delta;

		task->last_start[tix][cpu] = 0;
	}

	/* Reported switch event for task already on that CPU */
	if (i == task->monitored_current[cpu])
		return;

	/* Task entering the CPU */
	if (tix != i && i != task->monitored_nb) {
		task->last_start[i][cpu] = time;
	}

	/* Assign current task as running on that CPU */
	task->monitored_current[cpu] = i;
	if ((time < task->start) || (time > task->stop))
		return;

	printf("%lf", time);
	for (i = 0; i < task->max_cpu; i++)
		if (task->monitored_current[i] < task->monitored_nb)
			printf(",%d", task->monitored_pid[task->monitored_current[i]]);
		else
			printf(",-");
	printf("\n");
}

static void sched_switch_printall(void * data,int cpu, double time, long long *number, const char **str)
{
	int j;
	struct ftrace_so_process_matrix * task =
		(struct ftrace_so_process_matrix *)data;
	int pid = number[FTRACE_SO_NEXT_PID];

	if (task->max_cpu <= cpu)
		task->max_cpu = cpu + 1;

	if (pid == task->monitored_current[cpu])
		return;

	task->monitored_current[cpu] = pid;
	if ((time < task->start) || (time > task->stop))
		return;

	printf("%lf", time);
	for (j = 0; j < task->max_cpu; j++)
			printf(",%d", task->monitored_current[j]);
	printf("\n");
}

static const char * ftrace_dl_info(void)
{
	return "This libraries show timeline of pid set with PID=pid0[,pid1[,pid2[...]]] between START and STOP."
			"The environmment variable should be set like \"PID=pid0[,pid1[,pid2[...]]] START=start_time STOP=stop_time\"";
}

static void * ftrace_dl_init(void)
{
	int j;
	double temp[PID_MAX];
	struct ftrace_so_process_matrix * task =
		(struct ftrace_so_process_matrix *)
			malloc(sizeof(struct ftrace_so_process_matrix));
	if (task == NULL)
		return NULL;
	task->max_cpu = 0;
	task->monitored_nb = ftrace_helper_getenv_array("PID", &temp[0], PID_MAX);
	for (j = 0; j < task->monitored_nb; j++)
		task->monitored_pid[j] = temp[j];
	for (j = 0; j < CPU_MAX; j++)
		task->monitored_current[j] = task->monitored_nb;
	if (ftrace_helper_getenv_array("START", &task->start, 1) < 1)
		task->start = -HUGE_VAL;
	if (ftrace_helper_getenv_array("STOP", &task->stop, 1) < 1)
		task->stop = HUGE_VAL;
	if (task->monitored_nb <= 0)
		ftrace_decode_register("sched_switch", sched_switch_printall, task, ftrace_so_number, NULL);
	else
		ftrace_decode_register("sched_switch", sched_switch, task, ftrace_so_number, NULL);

	/* Load tasks residency checks */
	if (ftrace_helper_getenv_array_int("CPUS_MASK", &task->cpus_mask, 1) < 1)
		task->cpus_mask = 0;
	if (ftrace_helper_getenv_array("USAGE_MIN", &task->usage_min, 1) < 1)
		task->usage_min = 0.0;
	if (ftrace_helper_getenv_array("USAGE_MAX", &task->usage_max, 1) < 1)
		task->usage_max = 100.0;
	if (ftrace_helper_getenv_array("TIME_MIN", &task->time_min, 1) < 1)
		task->time_min = 0.0;
	if (ftrace_helper_getenv_array("TIME_MAX", &task->time_max, 1) < 1)
		task->time_max = -1;
	printf("Residency check configured for:\n");
	printf("  CPUS mask: 0x%X\n", task->cpus_mask);
	printf("  MIN usage: %4.1f%%\n", task->usage_min);
	printf("  MAX usage: %4.1f%%\n", task->usage_max);
	printf("  MIN time : %4.1f [s]\n", task->time_min);
	printf("  MAX time : %4.1f [s]\n", task->time_max);

	return task;
}

static int ftrace_dl_dumpusages(void * data)
{
	struct ftrace_so_process_matrix * task =
		(struct ftrace_so_process_matrix *)
			data;
	int idx, cpu;
	FILE *fd;
	double cpu_usage;
	unsigned mask = 0x1;

	fd = fopen("./tasks_residencies.txt", "w");
	if (fd == NULL) {
		printf("ERROR: failed opening file to store tasks residency\n"
			"(Error %d: %s)\n", errno, strerror(errno));
		return errno;
	}

	/* Dump tasks residencies table header */
	fprintf(fd, "Task(   PID ) ");
	for (cpu = 0; cpu < task->max_cpu; cpu++)
		fprintf(fd, "        CPU_%02d           ", cpu);
	fprintf(fd, "\n");

	/* Dump out tasks residencies per CPU */
	for (idx = 0; idx < task->monitored_nb; idx++) {
		fprintf(fd, "Task( %5d ): ", task->monitored_pid[idx]);
		for (cpu = 0; cpu < task->max_cpu; cpu++) {
			fprintf(fd, "%13.6f ( %5.1f %% )",
				task->time_total[idx][cpu],
				100 * task->time_total[idx][cpu] / task->task_total[idx]);
		}
		fprintf(fd, "\n");
	}

	/* Dump total time spent per CPU */
	fprintf(fd, "Totals         ");
	for (cpu = 0, mask = 0x1; cpu < task->max_cpu; cpu++, mask<<=1) {
		cpu_usage = 100.0 * task->cpus_total[cpu] / task->ovrl_total;
		fprintf(fd, "%13.6f ( %5.1f %% )", task->cpus_total[cpu], cpu_usage);

		/* Sum up expected CPUs usages */
		if (task->cpus_mask & mask) {
			task->usage_sum += cpu_usage;
			task->time_sum  += task->cpus_total[cpu];
			printf("Add CPU%d usage (%5.1f%%) => %5.1f%%\n",
					cpu, cpu_usage, task->usage_sum);
		}
	}
	fprintf(fd, "\n");

	printf("Expected MIN usage       : %5.1f%%\n", task->usage_min);
	printf("Expected MAX usage       : %5.1f%%\n", task->usage_max);
	printf("Actual (0x%02X) CPUs usage : %5.1f%%\n",
			task->cpus_mask, task->usage_sum);

	printf("Expected MIN time       : %9.6f [s]\n", task->time_min);
	printf("Expected MAX time       : %9.6f [s]\n", task->time_max );
	printf("Actual (0x%02X) CPUs time : %9.6f [s]\n",
			task->cpus_mask, task->time_sum);

	/* Checking for required usage match */
	if (task->usage_sum >= task->usage_min &&
			task->usage_sum <= task->usage_max) {
		task->residency_test_passed = true;
		printf("Residency test PASSED\n");
	} else {
		task->residency_test_passed = false;
		printf("Residency test FAILED\n");
	}

	/* Checking for required time match */
	if (task->time_sum < task->time_min ||
			(task->time_max >=0 &&
			 task->time_sum > task->time_max)) {
		task->time_test_passed = false;
		printf("Time test FAILED\n");
	} else {
		task->time_test_passed = true;
		printf("Time test PASSED\n");
	}

	fclose(fd);

	return 0;

}

static bool ftrace_dl_cleanup(void * data)
{
	struct ftrace_so_process_matrix * task =
		(struct ftrace_so_process_matrix *)
			data;

	if (task->monitored_nb)
		ftrace_dl_dumpusages(data);

	if (task == NULL)
		return false;
	free(data);

	if (!task->residency_test_passed || !task->time_test_passed)
		return false;

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

