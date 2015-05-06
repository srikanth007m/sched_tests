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

#define _GNU_SOURCE
#include "os.h"
#include <sched.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>



#define  OS_AVAILABEL_FREQ_MAX 64

static struct
{
	int cpu_nb;
	long long available_freq[OS_AVAILABEL_FREQ_MAX];
	long long available_freq_nb;
	const char * temperature_path;
} os_data;

static long long os_readfile_longlong(const char * suffix, int cpu)
{
	long long number;
	char t[256];
	FILE * fd;
	sprintf(t, "/sys/devices/system/cpu/cpu%d/%s", cpu, suffix);
	fd = fopen(t, "r");
	if (fd == NULL)
		return -1;
	if (fscanf(fd, "%lld", &number) < 1)
		number = -1;
	fclose(fd);
	return number;
}

static bool os_writefile_longlong(const char * suffix, int cpu, long long value)
{
	char t[256];
	FILE * fd;
	sprintf(t, "/sys/devices/system/cpu/cpu%d/%s", cpu, suffix);
	fd = fopen(t, "w");
	if (fd == NULL)
		return false;
	if (fprintf(fd, "%lld\n", value) < 1)
	{
		fclose(fd);
		return false;
	}
	fclose(fd);
	return true;
}

static bool os_writefile_cpu_string(const char * suffix, int cpu, const char * value)
{
	char t[256];
	FILE * fd;
	sprintf(t, "/sys/devices/system/cpu/cpu%d/%s", cpu, suffix);
	fd = fopen(t, "w");
	if (fd == NULL)
		return false;
	if (fprintf(fd, "%s\n", value) < 1)
	{
		fclose(fd);
		return false;
	}
	fclose(fd);
	return true;
}




void os_cleanup(void)
{
}

void os_init(enum os_init_t param)
{
	const char * path[] =
		{"/sys/class/thermal/thermal_zone0/temp", NULL};
	const char * * tmp = path;
	long long freq;
	long long max = os_readfile_longlong("cpufreq/cpuinfo_max_freq", 0);
	long long min = os_readfile_longlong("cpufreq/cpuinfo_min_freq", 0);
	FILE * fd;
	char t[80];

	os_data.cpu_nb = 0;

	while (1)
	{
		sprintf(t, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", os_data.cpu_nb);
		fd = fopen(t, "r");
		if (fd == NULL)
			break;
		fclose(fd);
		os_data.cpu_nb++;
	}


	while (*tmp != NULL)
	{
		fd = fopen(*tmp, "r");
		if (fd != NULL)
		{
			fclose(fd);
			break;
		}
		tmp++;
	}
	os_data.temperature_path = *tmp;

	os_data.available_freq_nb = 0;
	if (param != OS_INIT_ALL)
		return;
	os_data.available_freq_nb = 0;
	for (freq = min; freq <= max; freq += 50000)
	{
		if ((os_frequency_set(freq)) && (os_frequency_get() == freq))
		{
			os_data.available_freq[os_data.available_freq_nb] = freq;
			os_data.available_freq_nb++;
		}
	}
}

int os_cpu_nb(void)
{
	return os_data.cpu_nb;
}

double os_time(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + (tv.tv_usec / 1000000.0);
}


long long os_frequency_get(void)
{
	return os_readfile_longlong("cpufreq/scaling_cur_freq", 0);
}

bool os_frequency_set(long long freq)
{
	bool success = true;
	int cpu;
	for (cpu = 0; cpu < os_data.cpu_nb; cpu++)
	{
		long long max = os_readfile_longlong("cpufreq/scaling_max_freq", cpu);
		long long min = os_readfile_longlong("cpufreq/scaling_min_freq", cpu);
		long long curr = os_readfile_longlong("cpufreq/scaling_cur_freq", cpu);
		if ((min == freq) && (max == freq) && (curr == freq))
			continue;
		success &= os_writefile_cpu_string("cpufreq/scaling_governor", cpu, "powersave");
		if (freq <= max)
		{
			success &= os_writefile_longlong("cpufreq/scaling_min_freq", cpu, freq);
			success &= os_writefile_longlong("cpufreq/scaling_max_freq", cpu, freq);
		}
		else
		{
			success &= os_writefile_longlong("cpufreq/scaling_max_freq", cpu, freq);
			success &= os_writefile_longlong("cpufreq/scaling_min_freq", cpu, freq);
		}
		success &= os_writefile_cpu_string("cpufreq/scaling_governor", cpu, "performance");
	}
	return success;
}

void os_frequency_available(const long long * * freqs, int * freqs_nb)
{
	*freqs_nb = os_data.available_freq_nb;
	*freqs = os_data.available_freq;
}

bool os_temperature_get(double * temperatures, int nb)
{
	int i;
	double temperature;
	FILE * fd;
	if (nb != os_data.cpu_nb)
		return false;
	if (os_data.temperature_path == NULL)
		return false;
	fd = fopen(os_data.temperature_path, "r");
	if (fd == NULL)
		return false;

	if (fscanf(fd, "%lf", &temperature) < 1)
		temperature = -HUGE_VAL;
	else
		temperature /= 1000.0;
	fclose(fd);
	for (i = 0; i < nb; i++)
		temperatures[i] = temperature;
	if (temperature == -HUGE_VAL)
		return false;
	return true;
}

bool os_affinity_set(int cpu)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	if (cpu < 0)
	{
		int cpunb = os_cpu_nb();
		while (cpunb > 0)
		{
			cpunb--;
			CPU_SET(cpunb, &mask);
		}
		return sched_setaffinity(0,sizeof(mask), &mask) >= 0;
	}
	CPU_SET(cpu, &mask);
	return sched_setaffinity(0,sizeof(mask), &mask) >= 0;
}

void os_sleep(double seconds)
{
	struct timespec req;
	struct timespec rem;

	if (seconds <= 0)
		return;

	rem.tv_sec = (long)seconds;
	rem.tv_nsec = (long)((seconds - rem.tv_sec) * 1000000000);
	do
	{
		req = rem;
	} while (nanosleep(&req, &rem) < 0);
}

int os_child_create_deamon(void (*child)(int pid, void * data), void * data)
{
	int i;
	int pid = fork();
	if (pid != 0)
		return pid;
	/* close all file descriptors */
	for (i = 0; i < 1024; i++)
		close(i);
	/* detach from parent */
	i = setsid();
	child(getpid(), data);
	exit(0);
}

int os_child_create(void (*child)(int pid, void * data), void * data)
{
	int pid = fork();
	if (pid != 0)
		return pid;
	child(getpid(), data);
	exit(0);
}

void os_child_check(int pid, bool * exited, bool * success, int * cpu)
{
	int status;
	*cpu = -1;
	if (waitpid(pid, &status, WNOHANG) <= 0)
	{
		int i;
		char tmp[128];
		FILE * fd;
		*exited = false;
		*success = false;
		sprintf(tmp, "/proc/%d/stat", pid);
		fd = fopen(tmp, "r");
		if (fd == NULL)
			return;
		for (i = 0; i < 38; i++)
		{
			if (fscanf(fd, "%*[^ ]") < 0)
				return;

			if (fscanf(fd, "%*[ ]") < 0)
				return;
		}
		if (fscanf(fd, "%d", cpu) < 1)
			*cpu = -1;
		return;
	}
	if ((WIFEXITED(status)) && (WEXITSTATUS(status) == 0))
	{
		*exited = true;
		*success = true;
		return;
	}
	if ((WIFEXITED(status)) && (WEXITSTATUS(status) != 0))
	{
		*exited = true;
		*success = false;
		return;
	}

	if (WIFSIGNALED(status))
	{
		*exited = true;
		*success = false;
		return;
	}

	/* should never reach this */
	*exited = false;
	*success = false;
}

bool os_nice_set(int pid, int nice)
{
	errno = 0;
	if (setpriority(PRIO_PROCESS, pid, nice) == -1)
	{
		if (errno != 0)
			return false;
	}
	return true;
}

bool os_usage_get(double * system, double * user)
{
	struct rusage data;
	*system = 0;
	*user = 0;
	if (getrusage(RUSAGE_CHILDREN, &data) < 0)
		return false;
	*system = data.ru_stime.tv_sec + (data.ru_stime.tv_usec / 1000000.0);
	*user = data.ru_utime.tv_sec + (data.ru_utime.tv_usec / 1000000.0);
	return true;
}
