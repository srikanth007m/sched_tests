/*
 * test.c - framework to integrate scheduler test
 *
 * Created by:  Olivier Cozette, July 2012
 * Copyright:   (C) 2012  ARM Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdbool.h>
#include <math.h>



#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#include <sys/syscall.h>
#include <pthread.h>

#include <sys/time.h>
#include <stdlib.h>  // for NULL
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

static const char * error = "";

static bool switch_to(int cpu)
{
#ifdef ANDROID
    int err, syscallres;
    int mask = 0xffff;
    if (cpu >= 0)
        mask = 1 << cpu;
    syscallres = syscall(__NR_sched_setaffinity, 0 /* myself */, sizeof(mask), &mask);
    if (syscallres)
    {
        err = errno;
        return false;
    }
#else
    unsigned long mask = 0xffff;
    if (cpu >= 0)
        mask = 1 << cpu;
    if (sched_setaffinity(0, sizeof(mask), &mask) <0) 
        return false;
#endif
    return true;
}

/* TODO: Warning current implementation works only for monothreaded case */
static bool get_cpu(int * cpu)
{
	bool success = false;
	int i;
	char str_nb[64];
	FILE * fd = fopen("/proc/self/stat", "r");
	if (fd == NULL)
		return false;
	for (i = 0; i < 38; i++)
		if (fscanf(fd, "%s ", str_nb) < 1)
			break;
	if (i == 38)
		success = (fscanf(fd, "%d", cpu) > 0);

	fclose(fd);
	return success;
}	

static bool get_cpu_affinity(int * cpu)
{
#ifdef ANDROID
    int err, syscallres;
    int mask = 0xffff;
    syscallres = syscall(__NR_sched_getaffinity, 0 /* myself */, sizeof(mask), &mask);
    if (syscallres < 0)
    {
        err = errno;
        return false;
    }
#else
    unsigned long mask = 0xffff;
    if (sched_getaffinity(0, sizeof(mask), &mask) <0) 
        return false;
#endif
    if (((mask - 1) & mask) != 0)
	*cpu = -1;
    else
    {
	*cpu = 0;
	while (mask > 1)
	{
		mask = mask >> 1;
		*cpu = *cpu + 1; 
	}
    }
    return true;
}

#define THRESHOLD_USEC 2000

static double time_current(void)
{
        struct timeval tv;
	double res = 0;
        gettimeofday(&tv, NULL);
	res = tv.tv_sec;
	res = res + tv.tv_usec/1000000.0;
        return res;
}

static int cpu_max(void)
{
	int i;
	for (i = 0; i < 255; i++)
	{
                if (!switch_to(i))
                        return i;
        }
        return 0;
}

static double time_error(int seconds)
{
	double start = time_current();
	double last = start;
	double best = HUGE_VAL;
	double prev;
	do
	{
		prev = last;
		last = time_current();
		if ((last != prev) && (last - prev < best))
			best = last - prev;
	} while (last < start + seconds);
	return best;
}


volatile long long time_busy_notime_int = 0; /* used ot prevent any optimisation */
static void time_busy_notime(long long loop)
{
	/*
	 * This function is only core speed related, and not related to any subsystem like memory
	 */
	long long i;
	double x = 0.1;
	double y = 0.1;
	double temp;
	for (i = 0; i < loop; i++)
	{
		double nx = x*x-y*y + 0.1;
		double ny = 2*x*y + 0.1;
		if (nx*nx + ny*ny > 4)
		{
			time_busy_notime_int = i;
			return;/* will never happen because 0.1+0.1i is in mandelbrot set,
				 * but gcc don't know !*/
		}
		x = nx;
		y = ny;
	}
	time_busy_notime_int = i;
}

static double time_busy(long long loop)
{
	long long i;
	double x = 0.1;
	double y = 0.1;
	double temp;
	double start = time_current();
	time_busy_notime(loop);
	return (time_current() - start);
}

static double time_calibrate(double seconds)
{
	long long loop = 50;
	double time;
	usleep(500*1000); /* do a sleep to be sure we should get a full timeslice */
	do
	{
		loop = loop * 2;
		time = time_busy(loop);
	} while (time < seconds);
	return loop/time;
}

struct time_test_scheduler_struct
{
	volatile bool * can_start;
	volatile bool * finished;
	volatile bool * can_stop;
	bool result;
	double time_res;
	double time_migrate;
	double time_before_migrate;
	double seconds;
	int preempted;
	int cpu;
};

		
static void * time_test_cpufreq_scheduler_thread(void * args)
{
	struct time_test_scheduler_struct * data = (struct time_test_scheduler_struct *)args;
	double loop_per_seconds = time_calibrate(data->time_res * 1000);
	double min = HUGE_VAL;
	double max = -HUGE_VAL;
	double start;
	double last, prev, before_prev;
	double sum = 0;
	double time_slow = HUGE_VAL;
	double time_fast = HUGE_VAL;
	int i;
	int preempted = 0;
	double loop_time = 0.000200; /* 200us */
	int nb = 0;
	double time_migrate;

	data->result = false;
	if (!switch_to(data->cpu))
	{
		(*data->finished) = true;
		return 0;
	}

	/* Scheduler should migrated to slow CPU after these 2.5s */
	for (i = 0; i < 5; i++)
	{
		double time =  time_busy(loop_per_seconds * loop_time);
		if (time > 1.4 * loop_time)
		{
			/* the calibration was probably done on fast cpu, adapt it to slow one */
			loop_per_seconds = loop_per_seconds * loop_time / time;
		}
		if (loop_per_seconds * loop_time < data->time_res * 80)
		{
			/* if we can't measure the delay with good accuraty, we have to change it */
			loop_time = data->time_res * 100 / loop_per_seconds;
		}
		usleep(500*1000);
	}
	/* expect to be in slow cpu and have at least loop_time available in the */
	time_slow = time_busy(loop_per_seconds * loop_time);
	usleep(500*1000);

	/* improve time_slow computation, to avoid any time reminding to interrupt handler */
	for (i = 0; i < 1; i++)
	{
		double time_temp = time_busy(loop_per_seconds * loop_time);
		if (time_temp < time_slow)
			time_slow = time_temp;
		usleep(500*1000);
	}
	
	while (!(*data->can_start))
	{
		/* we have to wait, so use that to improve the time_slow accuraty */
		double time_temp = time_busy(loop_per_seconds * loop_time);
		if (time_temp < time_slow)
			time_slow = time_temp;
		usleep(500*1000);
	}

	start = time_current();
	last = start;
	prev = start;
	do
	{
		/* test at least if cpu is faster */
		before_prev = prev;
		prev = last;
		time_busy_notime(loop_per_seconds * loop_time);
		last = time_current();
		if (last - prev > 1.4 * time_slow)
			preempted++;
	} while ((last - start < data->seconds) && (last - prev >= 0.8 * time_slow));

	/*
         * Idea
         * time
	 * 0           to 0.5  sleep, so we might go to slow cpu
	 * 0.5         to 0.5002 compute time_slow, right if were are on slow cpu, run only for 200us, so should stay on slow CPU
	 * 0           to 0.5  sleep, so we might go to slow cpu
	 * 0.5         to 0.5002 compute time_slow, right if were are on slow cpu, run only for 200us, so should stay on slow CPU
	 *..........................................
         * .......     to start we should really be on the slow cpu, so time_slow is right
         * start       to .... time_busy ~ time_slow
	 * ........... to .... time_busy ~ time_slow
	 * ..........................................
         * ............to .... time_busy >> time_slow, time_busy ~ time to get back the timeslice
	 * ........... to .... time_busy ~ time_slow
	 * ..........................................
         * before_prev to prev time_busy >> time_slow, time_busy ~ time to migrate
         * prev        to last time_busy < time_slow, time_busy ~ time_fast
         */ 

	/* compute time_fast since the previous one could contains switch time */
	time_fast = time_busy(loop_per_seconds * loop_time);
	(*data->finished) = true;

	/* 
	 * the migrate happen between prev and before_prev or between last and prev
	 * - case 1:between prev and before_prev, in this case time_fast ~ last - prev
	 *		prev - before_prev = k * time_slow + time_migrate + (1-k)*time_fast
	 *		since time_slow > time_fast
	 *          	then time_migrate < prev - before_prev - time_fast
	 * - case 2:between last and prev, in this case time_fast << last - prev
	 *		last - prev = k * time_slow + time_migrate + (1-k)*time_fast
	 *		since time_slow > time_fast
	 *		then time_migrate < last - prev - time_fast
	 */ 
	if (time_fast < 0.8 * (last - prev))
		time_migrate = last - prev - time_fast;
	else
		time_migrate = prev - before_prev - time_fast;

	if (last - start >= data->seconds)
	{
		return NULL;
	}

	while (!(*data->can_stop))
		time_busy(loop_per_seconds * loop_time); /* do something to stay on fast cpu */
	
	if (time_fast > 0.8 * time_slow)
	{
		return NULL; /* the new cpu is as fast as the older one, so likely to be the same*/
	}
	
	if (prev != before_prev)
	{
		data->preempted = preempted;
		data->time_migrate = time_migrate;
		data->time_before_migrate = prev - start;
		data->result = true;
	}
	else
	{
		data->preempted = preempted;
		data->time_migrate = last - HUGE_VAL;
		data->time_before_migrate = -HUGE_VAL;
		data->result = false;
	}
	return NULL;
}


#define TIME_MAX_MIGRATE_TASK0_S 0.002
#define TIME_MAX_MIGRATE_TASK1_S 0.020

static bool set_cpu_data(const char * suffix, int i, const char * str)
{
	char t[128];
	FILE * fd;
	int ret;
	sprintf(t, "/sys/devices/system/cpu/cpu%d/%s", i, suffix);
	fd = fopen(t, "w");
	if (fd == NULL)
		return false;
	ret = fprintf(fd, "%s\n", str);
	fclose(fd);
	if (ret < 1)
		return false;
	return true;
}

static bool time_test_migration_scheduler(int seconds, double time_res)
{
	double time_migrate_sum1 = 0;
	double time_before_migrate_sum1 = 0;
	double time_migrate_sum2 = 0;
	double time_before_migrate_sum2 = 0;
	double time_out = 3;
	int nb1 = 0;
	int nb2 = 0;
	bool success = true;
	double start = time_current();
	error = "Cannot change affinity";
	if (switch_to(-1) < 0)
		return false;
	
	do
	{
		void * retval;
		pthread_t t1, t2;
		double time_migrate;
		double time_before_migrate;
		volatile bool start1 = false;
		volatile bool finished1 = false;
		volatile bool finished2 = false;
		volatile bool can_stop2 = false;
		struct time_test_scheduler_struct data1, data2;
		data1.time_res = time_res;
		data2.time_res = time_res;
		data1.seconds = time_out;
		data1.cpu = -1;
		data2.seconds = time_out;
		data2.cpu = -2;

		data1.can_start = &start1;
		data1.finished = &finished1;
		data2.can_start = &finished1;
		data2.finished = &finished2;
		data1.can_stop = &finished2;		
		data2.can_stop = &can_stop2;
	
		if (pthread_create(&t1, NULL, time_test_cpufreq_scheduler_thread, &data1) != 0)
			return false;
		if (pthread_create(&t2, NULL, time_test_cpufreq_scheduler_thread, &data2) != 0)
		{
			start1 = true;
			finished2 = true;
			pthread_join(t1, &retval);
			return false;
		}
		start1 = true;
		pthread_join(t1, &retval);
		can_stop2 = true;
		pthread_join(t2, &retval);
		if (!data1.result)
			continue;
		nb1++;
		time_migrate_sum1 += data1.time_migrate;
		time_before_migrate_sum1 += data1.time_before_migrate;
		
		if (!data2.result)
			continue;
		nb2++;
		time_migrate_sum2 += data2.time_migrate;
		time_before_migrate_sum2 += data2.time_before_migrate;
		printf("%lf %lf %lf %lf # time_before_migrate_up_cpu0 time_before_migrate_up_cpu1 time_to_migrate_cpu0 time_to_migrate_cpu1\n",
			data1.time_before_migrate, data2.time_before_migrate, data1.time_migrate, data2.time_migrate);
	} while ((nb1 == 0) || (time_current() - start < seconds));

	printf("#Result migrate_cpu0 %lf time_before_migrate_up %lf time_migrate\n", time_before_migrate_sum1/nb1, time_migrate_sum1/nb1);
	if (time_migrate_sum1/nb1 > TIME_MAX_MIGRATE_TASK0_S)
		success = false;
	if (nb2 > 0)
	{
		printf("#migrate_cpu1 %lf time_before_migrate_up %lf time_migrate\n", time_before_migrate_sum2/nb2, time_migrate_sum2/nb2);
		if (time_migrate_sum2/nb2 > TIME_MAX_MIGRATE_TASK1_S)
			success = false;
	}	
	fflush(stdout);
	error = "";
	return success;
}

static bool time_migrate(int seconds, int cpu_nb, double time_res)
{
	double min = HUGE_VAL;
	double max = -HUGE_VAL;
	double start;
	double last, prev;
	double sum = 0;
	int nb = 0;
	int cpu = 0;
	error = "Cannot change affinity";
	if (!switch_to(cpu))
		return false;
	start = time_current();
	last = start;
	do
	{
		prev = last;
		cpu = (cpu + 1) % cpu_nb;
		switch_to(cpu);
		last = time_current();
		nb++;
		sum += last - prev;
		if (last - prev > max)
			max = last - prev;
		if (last - prev < min)
			min = last - prev;
		
	} while (last - start < seconds);
	printf("%lf %lf #migration minavg maxavg\n", sum/nb, (last - start)/nb);
	printf("%lf %lf #migration min max\n", min, max);
	if (time_res * 4 > sum/nb)
	{
		error = "time res to small or no migrate\n";
		return false;
	}
	error = "";
	return true;
}


static bool time_setaffinity_stable(int seconds, double time_res, int cpu)
{
	double loop_per_seconds = time_calibrate(0.001);
	double time_slow = HUGE_VAL;
	double time_fast = HUGE_VAL;
	int i = 0;
	int cpu_temp;
	error = "Cannot change affinity";
	if (!switch_to(cpu))
		return false;

	/* Scheduler should migrated to slow CPU */
	time_busy(loop_per_seconds * 0.0001); /* 100us*/
	error = "Unexpected change of affinity";
	if (!get_cpu_affinity(&cpu_temp))
		return false;
	if (cpu_temp != cpu)
		return false;
	if (!get_cpu(&cpu_temp))
		return false;
	if (cpu_temp != cpu)
		return false;
	usleep(500*1000);
	time_busy(loop_per_seconds * 0.0001); /* 100us*/
	usleep(500*1000);
	time_busy(loop_per_seconds * 0.0001); /* 100us*/
	usleep(500*1000);
	/* expect to be in slow cpu */
	/* since we've just done a sleep, we should get a full timeslice, 
	   so 200us of work should be uninterrupted*/
	time_slow = time_busy(loop_per_seconds * 0.0002); /* 200us*/
	usleep(500*1000);
	/* expect to go to big cluster */
	time_fast = time_busy(loop_per_seconds * 2);

	error = "Unexpected change of affinity";
	if (!get_cpu_affinity(&cpu_temp))
		return false;
	if (cpu_temp != cpu)
		return false;
	if (!get_cpu(&cpu_temp))
		return false;
	if (cpu_temp != cpu)
		return false;
#if 0
	/* we seen a small break of time, so we should have migrated */
	for (int i = 0; i < 10; i++)
	{	/* we can't sleep to get the next timeslice since this could allow to migrate 
		 * so we choose to do 10 try and get the lowest value */
		double 	time_fast_temp = time_busy(loop_per_seconds * 0.0002); /* 200us*/
		if (time_fast_temp < time_fast)
			time_fast = time_fast_temp;
	}

	error = "Unexpected change of cpu speed";
	if (time_fast > 1.2 * time_slow)
		return false; /* the new cpu is as faster than the older one*/
	if (time_fast < 0.8 * time_slow)
		return false; /* the new cpu is as slower than the older one*/
#endif
	error = "";
	return true;
}


int main_migrate(int argc, char ** argv)
{
	double time_res = time_error(1);
	int cpu_nb = cpu_max();
	printf("%lf %d #migration time_res cpu_nb\n", time_res, cpu_nb);
	if (time_res > 0.0005)
		return 1;
	if (cpu_nb < 2)
		return 1;
	if (!time_migrate(60, cpu_nb, time_res))
	{
		fprintf(stderr, "%s", error);
		fflush(stderr);
		return 1;	
	}
	return 0;
}

int main_hmp_migration(int argc, char ** argv)
{
	double time_res = time_error(1);
	int cpu_nb = cpu_max();
	int cpu = 0;
	printf("%lf %d #migration time_res cpu_nb\n", time_res, cpu_nb);
	if (time_res > 0.0005)
		return 1;
	if (cpu_nb < 2)
		return 1;
	while (cpu < 1024)
	{
		if (!set_cpu_data("cpufreq/scaling_governor", cpu, "performance"))
			break;
		cpu++;
	}
	if (cpu < cpu_nb)
	{
		fprintf(stderr, "cannot set governor on cpu %d", cpu);
		fflush(stderr);
		return 1;
	}

	if (!time_test_migration_scheduler(50, time_res))
	{
		fprintf(stderr, "%s", error);
		fflush(stderr);
		return 1;	
	}
	return 0;
}

int main_affinity(int argc, char ** argv)
{
	int i;
	double time_res = time_error(1);
	int cpu_nb = cpu_max();
	printf("%lf %d #migration time_res cpu_nb\n", time_res, cpu_nb);
	if (time_res > 0.0005)
		return 1;
	if (cpu_nb < 2)
		return 1;
	for (i = 0; i < 20; i++)
	{
		if (!time_setaffinity_stable(50, time_res, i % cpu_nb))
		{
			fprintf(stderr, "%s", error);
			fflush(stderr);
			return 1;	
		}
	}
	return 0;
}

