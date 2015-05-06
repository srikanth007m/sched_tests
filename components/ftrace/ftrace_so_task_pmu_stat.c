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

/* ftrace_so_task_pmu_stat - library that looks for pmu stat data */

#include "ftrace_dl.h"
#include "ftrace_decode.h"
#include "ftrace_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Notice: all functions and variables must be static
 */

#define FTRACE_SO_TIME_NS 0
#define FTRACE_SO_INSTRUCTIONS 1
#define FTRACE_SO_MEMORY_ACCESS_BYTES 2
#define FTRACE_SO_INDEX 3
#define FTRACE_SO_PID 4
static const char * ftrace_so_number[] = {
	"time_ns", "instructions", "memory_access_bytes", "index", "pid", NULL};


#define FTRACE_SO_PREV_PID 0
#define FTRACE_SO_NEXT_PID 0
static const char * ftrace_so_number2[] = {
	"prev_pid", "next_pid", NULL};

#define FTRACE_SO_PREV_COMM 0
static const char * ftrace_so_str2[] = {
	"prev_comm", NULL};

#define CPU_MAX 5
#define INDEX_MAX 18
#define IPS_STEP (4LL*1024LL*1024LL)
#define IPS_MIN 0
#define IPS_STEP_MAX 4096

#define MPS_STEP (4LL*1024LL*1024LL)
#define MPS_MIN 0
#define MPS_STEP_MAX 4096

#define MPI_STEP (0.016)
#define MPI_MIN 0
#define MPI_STEP_MAX 1024

#define PERIOD_STEP (0.001)
#define PERIOD_MIN 0
#define PERIOD_STEP_MAX 4096

#define restrict

#define MEMORY_BANDWIDTH 1048576
#define MEMORY_BANDWIDTH_STEP 0.01

struct memory_bandwidth_entry
{
	double start;
	double number;
	int cpu;
};

struct memory_bandwidth
{
	struct
	{
		int index;
		double last;
                struct memory_bandwidth_entry current, prev;
        } cpu[CPU_MAX];
        FILE * fd_write;
        FILE * fd_read[CPU_MAX];
        int cpu_max;
};

static struct memory_bandwidth * memory_bandwidth_create(int cpu_max)
{
	int i;
	struct memory_bandwidth * mem =
		(struct memory_bandwidth *) malloc(sizeof(struct memory_bandwidth));
	if (mem == NULL)
		return NULL;
	if (cpu_max > CPU_MAX)
	{
		free(mem);
		return NULL;
	}

	for (i = 0; i < CPU_MAX; i++)
	{
                mem->cpu[i].index = 0;
                mem->cpu[i].last = -HUGE_VAL;
                mem->cpu[i].prev.start = -HUGE_VAL;
                mem->fd_read[i] = NULL;
        }
        mem->fd_write = fopen("tmp", "w");
        mem->cpu_max = 0;
        return mem;             
}

static void memory_bandwidth_destroy(struct memory_bandwidth * mem)
{
        int i;
        if (mem->fd_write != NULL)
                fclose(mem->fd_write);
        for (i = 0; i < mem->cpu_max; i++)
                if (mem->fd_read[i] != NULL)
                        fclose(mem->fd_read[i]);
        free(mem);
}

static void memory_bandwidth_add(struct memory_bandwidth * mem, double prevtime, double newtime, int cpu, double bytes)
{
	if (mem->fd_write == NULL)
		return;
	//if ((mem->cpu[cpu].last != -HUGE_VAL) && (mem->cpu[cpu].last < prevtime))
	{
		mem->cpu[cpu].current.cpu = cpu;
		mem->cpu[cpu].current.start = prevtime;
		mem->cpu[cpu].current.number = 0;
		fwrite(&mem->cpu[cpu].current, sizeof(mem->cpu[cpu].current), 1, mem->fd_write);
	}
	mem->cpu[cpu].current.cpu = cpu;
	mem->cpu[cpu].current.start = newtime;
	mem->cpu[cpu].current.number = bytes;
	if (newtime >  mem->cpu[cpu].last)
		fwrite(&mem->cpu[cpu].current, sizeof(mem->cpu[cpu].current), 1, mem->fd_write);
	mem->cpu[cpu].last = newtime;
	if (cpu >= mem->cpu_max)
		mem->cpu_max = cpu + 1;
}

static bool memory_bandwidth_getnext(FILE * fd, struct memory_bandwidth_entry * entry, int cpu, int * index)
{
	if (fseek(fd, *index, SEEK_SET) < 0)
		return false;
	if (ftell(fd) != *index)
		return false;
	while (fread(entry, sizeof(*entry), 1, fd) > 0)
	{
		if (entry->cpu != cpu)
			continue;
		*index = ftell(fd);
		return true;
	}
	entry->start = HUGE_VAL;
	entry->number = 0;
	return false;
}

static void memory_bandwidth_read_init(struct memory_bandwidth * mem, int index)
{
	int i;
	double max = HUGE_VAL;
        for (i = 0; i < mem->cpu_max; i++)
        {
                mem->cpu[i].index = index;
                memory_bandwidth_getnext(mem->fd_read[i], &mem->cpu[i].prev, i, &mem->cpu[i].index);
                memory_bandwidth_getnext(mem->fd_read[i], &mem->cpu[i].current, i, &mem->cpu[i].index);
                if (mem->cpu[i].current.start < max)
                        max = mem->cpu[i].prev.start;
                mem->cpu[i].current.number = 0;
	}
	/* tricks otherwise, we have no warranty that the 
		max(mem->cpu[i].prev.start) < min(mem->cpu[i].current.start) and it would defeat the algorithm*/
	for (i = 0; i < mem->cpu_max; i++)
		mem->cpu[i].prev.start = max;	
}

static bool memory_bandwidth_get(struct memory_bandwidth * mem, double * duration, double * rate, int * running)
{
	int i, cpu;
	double sum;
	double max, min;
	int not_idling = 0;
	if (mem->fd_write != NULL)
        {
                fclose(mem->fd_write);
                mem->fd_write = NULL;
                for (i = 0; i < mem->cpu_max; i++)
                {
                        mem->fd_read[i] = fopen("tmp", "r");
                        if (mem->fd_read[i] == NULL)
                                break;
                }
                if (i < mem->cpu_max)
                {
                        while (i > 0)
                        {
                                i--;
                                fclose(mem->fd_read[i]);
                                mem->fd_read[i] = NULL;
                        }
                        return false;
                }
                memory_bandwidth_read_init(mem, 0);
        }
        if (mem->fd_read[0] == NULL)
                return false;
        
        cpu = -1;
	max = HUGE_VAL; /* max = min(mem->cpu[i].current.start) */
	min = -HUGE_VAL; /* min = max(mem->cpu[i].prev.start) */
	for (i = 0; i < mem->cpu_max; i++)
	{
		if (mem->cpu[i].prev.start > min)
			min = mem->cpu[i].prev.start;
		if (mem->cpu[i].current.start < max)
		{
			cpu = i;
			max = mem->cpu[i].current.start;
		}
	}

	if ((max == HUGE_VAL) || (cpu == -1))
		return false;
	
	sum = 0;
	for (i = 0; i < mem->cpu_max; i++)
	{
		if (mem->cpu[i].current.number > 0)
			not_idling ++;
		sum += mem->cpu[i].current.number;
	}
	*duration = max - min;
        *rate = sum;

        mem->cpu[cpu].prev = mem->cpu[cpu].current;
        memory_bandwidth_getnext(mem->fd_read[cpu], &mem->cpu[cpu].current, cpu, &mem->cpu[cpu].index);

        if (mem->cpu[cpu].current.start < mem->cpu[cpu].prev.start)
                memory_bandwidth_read_init(mem, mem->cpu[cpu].index);
	*running = not_idling;
	return true;
}


struct ftrace_so_task_pmu_stat
{
        struct histogram * ips;
        struct histogram * mps;
        struct histogram * mpi;
        struct histogram * period;      
        struct memory_bandwidth * mem;
        
	int pid;
	struct {
		int capacity;
		int freq;
		int sorted;
	} index[INDEX_MAX];

	int cpuindex[CPU_MAX];
	double temp[CPU_MAX];
	double temp1[CPU_MAX];
	double temp2[CPU_MAX];
	char * title;
};

static void sched_task_pmu_stat_from_sysfs(const char * sysfs, struct ftrace_so_task_pmu_stat * task)
{
	int i, j;
	int index;
	int cap;
	int freq;
	for (i = 0; i < INDEX_MAX; i++)
	{
		task->index[i].capacity = -1;
		task->index[i].freq = 0;
		task->index[i].sorted = i;
	}
	while (sscanf(sysfs, "%d <%d %d>", &index, &freq, &cap)  >= 3)
	{
		task->index[index].capacity = cap;
		task->index[index].freq = freq * 1000;
		sysfs = strstr(sysfs, "> ");
		if (sysfs == NULL)
			break;
		sysfs += 2;
	}

	for (i = 0; i <= INDEX_MAX; i++)
		for (j = 0; j <= INDEX_MAX - 1; j++)
		{
			int temp;
			int index0 = task->index[j].sorted;
			int index1 = task->index[j + 1].sorted;
			if (task->index[index1].capacity < task->index[index0].capacity)
				continue;
			if ((task->index[index1].freq < task->index[index0].freq)
				&& (task->index[index1].capacity == task->index[index0].capacity))
				continue;
			temp = task->index[j].sorted;
			task->index[j].sorted = task->index[j + 1].sorted;
			task->index[j + 1].sorted = temp;
		}
}

static void sched_switch(void * data,int cpu, double time, long long * number, const char **str)
{
	int prev_pid = number[FTRACE_SO_PREV_PID];
	int next_pid = number[FTRACE_SO_NEXT_PID];
	struct ftrace_so_task_pmu_stat * task =
		(struct ftrace_so_task_pmu_stat *)data;
	double last_running;
	double last_waiting;
	if ((prev_pid == task->pid) && (task->title == NULL))
		task->title = strdup(str[FTRACE_SO_PREV_COMM]);
	task->temp2[cpu] = task->temp1[cpu];
	task->temp1[cpu] = time;

	if ((task->pid != -1) && (task->pid != next_pid))
		return;
	if ((ftrace_helper_task_waiting(next_pid, &last_running, &last_waiting)) && (last_waiting > 0))
		histogram_add(task->period, task->cpuindex[cpu], last_waiting + last_running, last_waiting + last_running);
}

static void sched_task_pmu_stat(void * data,int cpu, double time, long long * number, const char **str)
{
	struct ftrace_so_task_pmu_stat * task =
		(struct ftrace_so_task_pmu_stat *)data;
	double time_ns = number[FTRACE_SO_TIME_NS];
	double instructions = number[FTRACE_SO_INSTRUCTIONS];
	long long memory_access_bytes = number[FTRACE_SO_MEMORY_ACCESS_BYTES];
	int index = number[FTRACE_SO_INDEX];
	int pid = number[FTRACE_SO_PID];

	double time_s = time_ns/1000.0/1000.0/1000.0;
	double rate = instructions/time_s;

	task->cpuindex[cpu] = index;

	if (task->index[index].capacity < 0)
	{
		long long freq;
		int pid;
		if (ftrace_helper_cpu_info(cpu, &pid, &freq))
		{
			task->index[index].capacity = (cpu < 2)?2:1;
			task->index[index].freq = freq;
                                
                }               
        }
        memory_bandwidth_add(task->mem, time - time_s, time, cpu, memory_access_bytes/time_s);
        if ((task->pid != -1) && (task->pid != pid))
                return;
        histogram_add(task->mps, index, time_s, memory_access_bytes/time_s);
        histogram_add(task->mpi, index, time_s, memory_access_bytes/instructions);
        histogram_add(task->ips, index, time_s, rate);
        task->temp[cpu] = time;
}

static const char * ftrace_dl_info(void)
{
	return "This libraries does statistics about instructions/s when sched_task_pmu_stat trace event is present. Always success";
}

static void * ftrace_dl_init(void)
{
	int pid;
	int index;
	int cpu;
	struct ftrace_so_task_pmu_stat * task =
		(struct ftrace_so_task_pmu_stat *)
			malloc(sizeof(struct ftrace_so_task_pmu_stat));
	if (task == NULL)
		return NULL;
	task->title = NULL;
	for (index = 0; index < INDEX_MAX; index++)
	{
		task->index[index].capacity = -1;
		task->index[index].freq = -1;
		task->index[index].sorted = index;
	}

	for (cpu = 0; cpu < CPU_MAX; cpu++)
		task->cpuindex[cpu] = 0;

        task->ips = histogram_create(INDEX_MAX, IPS_STEP_MAX, IPS_MIN, IPS_STEP);
        task->mps = histogram_create(INDEX_MAX, MPS_STEP_MAX, MPS_MIN, MPS_STEP);
        task->mpi = histogram_create(INDEX_MAX, MPI_STEP_MAX, MPI_MIN, MPI_STEP);
        task->period = histogram_create(INDEX_MAX, PERIOD_STEP_MAX, PERIOD_MIN, PERIOD_STEP);
        task->mem = memory_bandwidth_create(5);
        if ((task->mem == NULL) || (task->period == NULL)
                || (task->mpi == NULL) || (task->mps == NULL) || (task->ips == NULL))
        {
                memory_bandwidth_destroy(task->mem);
                histogram_destroy(task->period);
                histogram_destroy(task->mpi);
                histogram_destroy(task->mps);
                histogram_destroy(task->ips);
                free(task);
		return NULL;
	}

	ftrace_decode_register("sched_task_pmu_stat", sched_task_pmu_stat, task, ftrace_so_number, NULL);
	ftrace_decode_register("sched_switch", sched_switch, task, ftrace_so_number2, ftrace_so_str2);
	if (ftrace_helper_getenv_array_int("PID", &pid, 1) < 1)
		pid = -1;
	if (pid == -1)
		task->title = strdup("All pids");

	task->pid = pid;

	/*sched_task_pmu_stat_from_sysfs(
"0 <1200000 2> 1 <1000000 2> 2 <700000 1> 3 <350000 1> 4 <1000000 1> 5 <500000 2> 6 <800000 1> 7 <900000 1> 8 <900000 2> 9 <500000 1> 10 <600000 1> 11 <600000 2> 12 <700000 2> 13 <800000 2> 14 <1100000 2> 15 "
		, task);*/

	return task;
}

static void ftrace_gnuplot_add(struct ftrace_so_task_pmu_stat * task,

			const char * title0,
			const char * title1,
			const char * filename,
			FILE * gnuplot,
			struct histogram * histo)
{
        int i;
        int j = 0;
        int index_not_null = 0;

        if (histo == NULL)
        {
                fprintf(gnuplot, "C=0.125\nset title \"%s %s\"\nplot", title0, title1);
                fprintf(gnuplot, "\"%s\" using ($1):($2) with lines notitle",
                                filename);
        }
        else
        {
                printf("#--- %s %s ---\n", title0, title1);
                for (i = 0; i < INDEX_MAX; i++)
                {
			double time;
			double avg;
			int index;
			index = task->index[i].sorted;
			histogram_info(histo, index, &time, &avg);
			if (time < 0.001)
				continue;
			printf("#--- %2d freq %12d cap %d average %10.0lf time %3.3lf\n", 
				index,
				task->index[index].freq,
				task->index[index].capacity,
				avg,
				time);
			index_not_null++;
		}
		fprintf(gnuplot, "C=0.125\nset title \"%s %s\"\nplot", title0, title1);
		j = 0;
		for (i = 0; i < INDEX_MAX; i++)
		{
			double time;
			double avg;
			int index;
			index = task->index[i].sorted;
			histogram_info(histo, index, &time, &avg);
			if (time < 0.001)
				continue;
			if (index_not_null > 1)
				fprintf(gnuplot, "%s\\\n\"%s\" using ($1):($%d+%d*C) with lines title \"capa %d freq %d\"",
				(j==0)?"":",",
				filename,
				index + 2,
				index_not_null - j - 1,
				task->index[index].capacity,
				task->index[index].freq);
			else
				fprintf(gnuplot, "%s\\\n\"%s\" using ($1):($%d) with lines notitle",
					(j==0)?"":",",
					filename,
                                        index + 2);
                        j++;
                }
        }
        fprintf(gnuplot, "\npause -1\n");
}

static bool ftrace_dl_cleanup(void * data)
{
	char t[256];
	int index;
	int i, j;
	double mem_duration;
	double mem_rate;
        struct ftrace_so_task_pmu_stat * task =
                (struct ftrace_so_task_pmu_stat *)data;
        FILE * fd;
        FILE * fd_mem;
        struct histogram * global;
        struct histogram * running;
        if (data == NULL)
		return false;
	if (task->title == NULL)
		task->title = strdup("Unknown process");

	for (i = 0; i < INDEX_MAX; i++)
		for (j = 0; j < INDEX_MAX - 1; j++)
		{
			int temp;
			int index0 = task->index[j].sorted;
			int index1 = task->index[j + 1].sorted;
			if (task->index[index1].capacity < task->index[index0].capacity)
				continue;
			if ((task->index[index1].freq < task->index[index0].freq)
				&& (task->index[index1].capacity == task->index[index0].capacity))
				continue;
			temp = task->index[j].sorted;
			task->index[j].sorted = task->index[j + 1].sorted;
			task->index[j + 1].sorted = temp;
		}

	fd = fopen("all.dem", "w");
	sprintf(t, "%sresult_ips.dat", task->title);
	histogram_dump(task->ips, t);
        ftrace_gnuplot_add(task, task->title, "Instruction per second", t, fd, task->ips);
        histogram_destroy(task->ips);

        sprintf(t, "%sresult_mpi.dat", task->title);
        histogram_dump(task->mpi, t);
        ftrace_gnuplot_add(task, task->title, "Byte per instruction", t, fd, task->mpi);
        histogram_destroy(task->mpi);

        sprintf(t, "%sresult_mps.dat", task->title);
        histogram_dump(task->mps, t);
	ftrace_gnuplot_add(task, task->title, "memory per second", t, fd, task->mps);
	histogram_destroy(task->mps);

	sprintf(t, "%sresult_period.dat", task->title);
	histogram_dump(task->period, t);
	ftrace_gnuplot_add(task, task->title, "perio", t, fd, task->period);
	histogram_destroy(task->period);


        global = histogram_create(1, MPS_STEP_MAX, MPS_MIN, MPS_STEP);
        running = histogram_create(1, 6, 0, 1);

        sprintf(t, "%smemory_over_time.dat", task->title);
        fd_mem = fopen(t, "w");

        if ((global != NULL) && (running != NULL) && (fd_mem != NULL))
        {
                int run;
                double time_last = 0;
                double time_current = 0;
                double rate_sum = 0;
                while (memory_bandwidth_get(task->mem, &mem_duration, &mem_rate, &run))
                {
                        if (run > 0)
                        {
                                rate_sum += mem_rate * mem_duration;
                                time_current += mem_duration;
                                histogram_add(running, 0, mem_duration, run);
                        
                                if (time_current > 0.01 + time_last)
                                {
                                        double elapsed = (time_current - time_last);
                                        double rate_avg = rate_sum / elapsed;
                                        histogram_add(global, 0, elapsed, rate_avg);

                                        fprintf(fd_mem, "%lf %lf\n", time_last, rate_avg);
                                        time_last = time_current;
                                        rate_sum = 0;
                                }
                        }
                }
                fclose(fd_mem);
                ftrace_gnuplot_add(task, "Total memory per second", "Memory over time", t, fd, NULL);

                histogram_dump(global, "result_mps_all.dat");
                ftrace_gnuplot_add(task, task->title, 
                        "Total memory per second", "result_mps_all.dat", fd, global);
		histogram_destroy(global);
		histogram_dump(running, "result_running_all.dat");
		ftrace_gnuplot_add(task, task->title, 
			"Running CPU", "result_running_all.dat", fd, running);
		histogram_destroy(running);
	}
	memory_bandwidth_destroy(task->mem);

	free(task->title);
	fclose(fd);
	free(task);
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

