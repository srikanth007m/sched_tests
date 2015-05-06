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
 * main - Launch a bunch of tasks and telling when there are finished
 */

#include "os.h"
#include "busy_loop.h"
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <math.h>


struct busy_loop_data * busy_init(double running_time)
{
	long long loop = 500;
	double time;
	struct busy_loop_data * busy = busy_loop_create(1000);
	if (busy == NULL)
		return NULL;

	/* stick to a cpu */
	os_affinity_set(0);
	time = os_time();

	/* run for 1s to be sure to be at max frequency */
	while (os_time() < time + 2)
		assert(busy_loop_run(busy));
	do
	{
		busy_loop_destroy(busy);
		loop = loop * 2;
		busy = busy_loop_create(loop);
		if (busy == NULL)
		{
			os_affinity_set(-1);
			return NULL;
		}
		time = os_time();
		assert(busy_loop_run(busy));
	} while (os_time() - time < running_time);

	/* no more stick to any CPU */
	os_affinity_set(-1);
	return busy;
}

void busy_run(long long loop, double sleep, struct busy_loop_data * busy_loop)
{
	while (loop > 0)
	{
		assert(busy_loop_run(busy_loop));
		os_sleep(sleep);
		loop--;
	}
}

struct busy_data
{
	/* global parametters */
	double total_sequential_time;
	double running_time;
	double sleeping_time;
	char ** tasks_nice;
	int tasks_max;

	/* global used in each subtask */
	struct busy_loop_data * busy_loop;

	/* local private used in each subtask */
	int nice;

	/* global for start and stop management of subtask */
	const char * error_str;
	int * pids;
	int pids_nb;
	double * times;

	double time;
	double time_user;
};

void main_cleanup(struct busy_data * busy);

void main_child(int pid, void * data)
{
	struct busy_data * busy =
		(struct busy_data *)data;

	os_sleep(1); /* allows the other fork to happen ASAP */
	os_nice_set(pid, busy->nice);

	busy_run((int)(busy->total_sequential_time / (busy->running_time + busy->sleeping_time)),
			busy->sleeping_time,
			busy->busy_loop);
	main_cleanup(busy);
}

/*
 * Init all parameters
 */
bool main_init(struct busy_data * busy, int argc, char ** argv)
{
	memset(busy, 0, sizeof(struct busy_data));
	busy->total_sequential_time = atof(argv[1]);
	busy->running_time = atof(argv[2]);
	busy->sleeping_time = atof(argv[3]);

	busy->tasks_max = argc - 4;
	busy->tasks_nice = argv + 4;

	busy->error_str = "Invalid combination of TOTAL_SEQUENTIAL_TIME RUNNING_TIME SLEEPING_TIME";
	if ((busy->total_sequential_time / (busy->running_time + busy->sleeping_time) <= 0)
		|| (busy->total_sequential_time / (busy->running_time + busy->sleeping_time) > 1024*1024*1024))
		return false;

	busy->error_str = "Not enough memory";
	busy->times = (double *)malloc(sizeof(double) * (argc - 4));
	if (busy->times == NULL)
		return false;

	busy->pids = (int *)malloc(sizeof(int) * (argc - 4));
	if (busy->pids == NULL)
		return false;

	busy->busy_loop = busy_init(busy->running_time);
	if (busy->busy_loop == NULL)
		return false;
	busy->error_str = NULL;
	return true;
}

/*
 * Free all parameters
 */
void main_cleanup(struct busy_data * busy)
{
	if (busy->times != NULL)
		free(busy->times);
	if (busy->busy_loop != NULL)
		busy_loop_destroy(busy->busy_loop);
	if (busy->pids)
		free(busy->pids);
}

/*
 * Launch all process
 */
bool main_launch(struct busy_data * busy)
{
	double start_user;
	double start_system;
	busy->pids_nb = 0;

	busy->time = os_time() + 1; /* because everyone will do a os_sleep(1) at the start */
	busy->time_user = 0;
	if (!os_usage_get(&start_system, &start_user))
	{
		printf("# Can't get ressource usage\n");
		return false;
	}
	busy->time_user = start_system + start_user;;

	for (busy->pids_nb = 0; busy->pids_nb < busy->tasks_max; busy->pids_nb++)
	{
		busy->nice = atol(busy->tasks_nice[busy->pids_nb]);
		busy->pids[busy->pids_nb] = os_child_create(main_child, busy);
		if (busy->pids[busy->pids_nb] < 0)
		{
			printf("Can't create more than %d process\n", busy->pids_nb);
			return false;
		}
	}
	return true;
}

/*
 * Wait end of all process
 */
bool main_wait(struct busy_data * busy)
{
	int j;
	int tasks = busy->pids_nb;
	double stop_user;
	double stop_system;
	bool success_all = true;

	while (tasks > 0)
	{
		os_sleep(1); /* Waiting from sigalarm + sigchld may be better */
		for (j = 0;j < busy->pids_nb; j++)
		{
			bool success;
			bool exited;
			int cpu;
			if (busy->pids[j] <= 0)
				continue;
			os_child_check(busy->pids[j], &exited, &success, &cpu);
			if (!exited)
				continue;
			if (!success)
			{
				printf("Task %d nice %s unsuccesfull\n", busy->pids[j], busy->tasks_nice[j]);
				success_all = false;
			}
			tasks--;
			busy->pids[j] = -1;
			busy->times[j] = os_time() - busy->time;

		}
		if (os_usage_get(&stop_system, &stop_user))
			printf("%lf %lf %d\n", os_time(), stop_system + stop_user - busy->time_user, tasks);
		else
			exit(1); /* worked in main_start(0 and not here ???? */
	}
	busy->time_user = stop_system + stop_user - busy->time_user;
	busy->time = os_time() - busy->time;
	return success_all;

}

/*
 * Check results
 */
bool main_check(struct busy_data * busy)
{
	bool success = true;
	int j;
	int nice_higher = 1024;
	int nice_current = -1024;
	int nice;
	double nice_higher_avg = HUGE_VAL;


	printf("#Running time\n#\t");
	for (j = 0;j < busy->pids_nb; j++)
	{
		nice = atol(busy->tasks_nice[j]);
		printf(" %3.0lf (%d)", busy->times[j], nice);
		if (nice > nice_current)
			nice_current = nice;
	}
	printf("\n");

	while (nice_current > -1024)
	{
		int nice_lower = -1024;
		double nice_avg = 0;
		int nb = 0;
		/* step 1 compute average */
		for (j = 0; j < busy->pids_nb; j++)
		{
			nice = atol(busy->tasks_nice[j]);
			if (nice > nice_current)
				continue;
			if (nice == nice_current)
			{
				nb++;
				nice_avg += busy->times[j];
				continue;
			}
			if (nice < nice_lower)
				continue;
			nice_lower = nice;
		}
		nice_avg /= nb;

		if (nice_avg > 0.9 * nice_higher_avg)
		{
			printf("#\tUnbalanced group with nice %d is %3.0lf%% of group with nice %d\n", nice_current, 100.0 * nice_avg / nice_higher_avg, nice_higher);
			success = false;
		}

		/* step 2 check with average */
		for (j = 0; j < busy->pids_nb; j++)
		{
			nice = atol(busy->tasks_nice[j]);
			if (nice != nice_current)
				continue;
			if ((busy->times[j] > 1.15 * nice_avg)
				|| (busy->times[j] < 0.85 * nice_avg))
			{
				printf("#\tUnbalanced %d nice %s: time is %3.0lf%% of average (%3.0lf) for this nice\n", j, busy->tasks_nice[j], 100.0 * busy->times[j] / nice_avg, nice_avg);
				success = false;
			}
		}

		printf("#Group with nice %d with %d process average time is %lf\n", nice_current, nb, nice_avg);
		nice_higher = nice_current;
		nice_current = nice_lower;
		nice_higher_avg = nice_avg;
	}

	printf("# Ran for %d * %3.0lf = %3.0lf cpu.s Application ran for %3.0lf \n",
		os_cpu_nb(),
		busy->time,
		busy->time * os_cpu_nb(),
		busy->time_user);
	printf("#\t%3.0f%% of available time \n",
		100.0 * busy->time_user / busy->time / os_cpu_nb());
	if (busy->time_user < 0.8 * busy->time * os_cpu_nb())
	{
		printf("#\terror used less than 80%% of cpus\n");
		success = false;
	}
	return success;
}

int main(int argc, char ** argv)
{
	bool success_all = true;
	bool unbalance_check = true;
	struct busy_data busy;
	const char * exec_name = argv[0];

	os_init(OS_INIT_NOFREQUENCY);
	printf("# %s Created by Olivier Cozette <olivier.cozette@arm.com>\n# Copyright (C) 2013  ARM Limited\n", argv[0]);

	if ((argc > 1) && (strcmp(argv[1], "NO_BALANCE_CHECK") == 0))
	{
		argv++;
		argc--;
		unbalance_check = false;
	}

	if (argc < 5)
	{
		printf("#Usage %s [NO_BALANCE_CHECK] TOTAL_SEQUENTIAL_TIME RUNNING_TIME SLEEPING_TIME NICE_PID0 [NICE_PID1] ...\n", exec_name);
		printf("#\tif NO_BALANCED_CHECK is given, even unbalanced thread won't raise a FAILED\n");
		os_cleanup();
		exit(1);
	}

	if (!main_init(&busy, argc, argv))
	{
		printf("# error :%s\n", busy.error_str);
		main_cleanup(&busy);
		os_cleanup();
		exit(1);
	}

	if (!main_launch(&busy))
		success_all = false;
	if (!main_wait(&busy))
		success_all = false;
	if (success_all)
		printf("SUCCESS before checking if tasks were well balanced\n");
	if (!main_check(&busy))
	{
		if (unbalance_check)
			success_all = false;
		else
			printf("#Seen as unbalanced or not using all the cpu time, but this doesn't affect the result\n");
	}

	main_cleanup(&busy);
	os_cleanup();
	if (!success_all)
	{
		printf("FAILED\n");
		exit(1);
	}
	printf("SUCCESS\n");
	return 0;
}
