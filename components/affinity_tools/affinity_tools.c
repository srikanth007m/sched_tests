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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <stdlib.h>  // for NULL
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>

#define CPU_MAX 32
int hog_cpu_pid_nb;
int hog_cpu_pids[CPU_MAX];

void hog_cpu_kill(int sig)
{
	int i;
	for (i = 0; i < hog_cpu_pid_nb; i++)
		kill(hog_cpu_pids[i], SIGKILL);
	exit(0);
}

bool setaffinity(int cpu, int pid)
{
	int mask = 1 << cpu;
	int res;
	res = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
	if (res < 0)
		return false;
	return true;
}

bool setmyaffinity(int cpu)
{
	return setaffinity(cpu, 0);
}

void hog_cpu(int nb, int * cpu)
{
	hog_cpu_pid_nb = 0;
	signal(SIGUSR1, hog_cpu_kill);
	while (nb > 1)
	{
		nb--;
		hog_cpu_pids[hog_cpu_pid_nb] = fork();
		if (hog_cpu_pids[hog_cpu_pid_nb] == 0)
		{
			printf("#HOG on cpu %d\n", cpu[nb]);
			if (!setmyaffinity(cpu[nb]))
				exit(1);
			while (1)
				;
		}
		hog_cpu_pid_nb++;
	}
	nb--;
	if (!setmyaffinity(cpu[nb]))
		exit(1);
	while (1)
		;

}

void task_set(int nb, int * cpu, int pid)
{
	int mask = 0;
	int res, err;
	printf("# Set affinity of pid %d to cpu", pid);
	while (nb > 0)
	{
		nb--;
		mask |= (1 << cpu[nb]);
		printf(" %d", cpu[nb]);
	}
	printf("\n");
	res = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
	if (res < 0)
		printf("# Can't set affinity of pid %d to %d\n", pid, mask);
}


bool read_midr(int cpu, unsigned int * implementer, unsigned int * part)
{
	FILE * fd;
	bool all_processor = false;
	int current_processor = -1;
	char str[256];
	*implementer = 0;
	*part = 0;
	if (!setmyaffinity(cpu))
		return false;
	fd = fopen("/proc/cpuinfo", "r");
	if (fd == NULL)
		return false;
	while ((*part == 0) || (*implementer == 0))
	{
		char * number;
		while (fscanf(fd, "%*[\n]") > 0)
			;
		if (fscanf(fd, "%[^\n]", str) < 1)
			break;
		if (strncmp(str, "processor", strlen("processor")) == 0)
		{
			all_processor = true;
			sscanf(strchr(str, ':') + 2, "%d", &current_processor);
		}
		if ((all_processor) && (cpu != current_processor))
			continue;
		if ((strstr(str, "CPU part") == NULL)
			&& (strstr(str, "CPU implementer") == NULL))
			continue;
		number = strchr(str, ':') + 1;
		if (number == NULL)
			continue;
		while (*number == ' ')
			number++;
		if (strstr(str, "CPU part") != NULL)
			sscanf(number, "0x%x", part);
		else
			sscanf(number, "0x%x", implementer);
	}
	fclose(fd);
	if ((*part == 0) || (*implementer == 0))
		return false;
	return true;
}


int main(int argc, char **argv)
{
	int nb;
	int cpu[CPU_MAX];
	int taskset = false;
	const char * cpu_list;
	printf("%s Created by:  Olivier Cozette, November 2012 Copyright:   (C) 2012  ARM Limited\n", argv[0]);
	if (argc < 2)
	{
		printf("Usage:\n");
		printf("\t CPU\t\tWill hog the cpu CPU cpu is a comma separated list\n");
		printf("\t -pc CPU PID\tWill set affinity of PID to CPU is a comma separated list\n");
		printf("\t -part CPU,IMPLEMENTER,PART\tWill exit 0 if the CPU is this one or exit 1 otherwise\n");
		printf("Example: \"%s -part 0,0x41,0xc07\" will exit 0 if the CPU 0 is an A7\n", argv[0]);
		printf("Example: \"%s -part 0,0x41,0xc0f\" will exit 0 if the CPU 0 is an A15\n", argv[0]);
		printf("Example: \"%s -part 0,0x41,0xd03\" will exit 0 if the CPU 0 is an A53\n", argv[0]);
		printf("Example: \"%s -part 0,0x41,0xd07\" will exit 0 if the CPU 0 is an A57\n", argv[0]);
		printf("\t -showpart\tWill display part for all CPU\n");
		printf("Example: \"%s -showpart\" \n0,0x41,0xc0f,1,0x41,0xc0f,2,0x41,0xc07,3,0x41,0xc07,4.0x41,0xc07\n", argv[0]);
		printf("Meaning CPU0,1 are A15 and CPU2,3,4 are A7\n");
		printf("Example: \"%s -showpart\" \n0,0x41,0xd03,1,0x41,0xd07,2,0x41,0xd07,3,0x41,0xd03,4.0x41,0xd03,5.0x41,0xd03\n", argv[0]);
		printf("Meaning CPU0,3,4,5 are A53 and CPU1,2 are A57\n");
		return 0;
	}

	cpu_list = argv[1];
	if ((strcmp(argv[1], "-part") == 0) && (argc > 2))
	{
		unsigned int cpu, part, implementer;
		unsigned int real_part, real_implementer;
		if (sscanf(argv[2], "%d,0x%x,0x%x", &cpu, &implementer, &part) < 3)
		{
			printf("The option \"-part\" need to be followed by \"CPU,IMPLEMENTER,PART\" and it was <%s>\n", argv[2]);
			printf("where CPU is an integer and where IMPLEMENTER and PART are in the form 0x[0-9]*\n");
			exit(1);
		}
		if (!read_midr(cpu, &real_implementer, &real_part))
			exit(1);
		if ((real_implementer != implementer) || (real_part != part))
			exit(1);
		exit(0);
	}
	if (strcmp(argv[1], "-showpart") == 0)
	{
		bool first = true;
		unsigned int cpu, part, implementer;
		for (cpu = 0; cpu < 31; cpu++)
		{
			if (!read_midr(cpu, &implementer, &part))
				continue;
			if (!first)
				printf(",");
			first = false;
			printf("%d,0x%x,0x%x", cpu, implementer, part);
		}
		printf("\n");
		exit(0);
	}
	if (strcmp(argv[1], "-pc") == 0)
	{
		taskset = true;
		if (argc < 4)
		{
			printf("need to specify CPU list and PID\n");
			return 1;
		}
		cpu_list = argv[2];
	}
	nb = sscanf(cpu_list, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		&cpu[0], &cpu[1], &cpu[2], &cpu[3], &cpu[4], &cpu[5], &cpu[6], &cpu[7],
		&cpu[8], &cpu[9], &cpu[10], &cpu[11], &cpu[12], &cpu[13], &cpu[14], &cpu[15]);

	if (taskset)
		task_set(nb, cpu, atol(argv[3]));
	else
		hog_cpu(nb, cpu);

	return 0;
}
