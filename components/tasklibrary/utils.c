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

/* utils - utility functions for load generator */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include "timerload.h"

static const char cpu_governor_name[]="/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor";

int get_governor(int cpu, char* governor, int governor_len)
{
	char filename[512];
	int result=0;
	FILE *file;
	sprintf(filename, cpu_governor_name, cpu);
	file = fopen(filename, "r");
	if(file){
		result = fread(governor, 1, governor_len, file);
		fclose(file);
	}
	return result;
}
int set_governor(int cpu, char* governor)
{
	char filename[512];
	int result=0;
	FILE *file;
	sprintf(filename, cpu_governor_name, cpu);
	file = fopen(filename, "w");
	if(file){
		result = fwrite(governor, 1, strlen(governor), file);
		fclose(file);
	}
	return result;
}

/*
 * From the loaded calibration data, return the CPUID of the CPU with
 * the highest number of iterations in big & the CPUID of the CPU
 * with the lowest number of iterations in little
 *
 * Used to transform 'big' and 'little' CPU identifiers into a concrete
 * CPUID
 */
void get_big_little_cpuids(int *big, int *little)
{
	int cpu=0;
	*big = 0;
	*little = 0;
	unsigned int big_count=0, little_count=0xFFFFFFFF;
	/* find big and little CPUs */
	while(calibration_data[cpu].calibration_points)
	{
		int data_idx = (calibration_data[cpu].calibration_points - 1);
		int loops = calibration_data[cpu].data[data_idx].loops;
		if(loops > big_count)
		{
			big_count = loops;
			*big = cpu;
		}
		if(loops < little_count)
		{
			little_count = loops;
			*little = cpu;
		}
		cpu++;
	}
}

/*
 * Based upon the loaded CPU calibration data, convert the CPU identifier
 * from the load script into a present CPU.
 *
 * big & little are the identified CPUIDs for big/little CPUs respectively.
 * cpustr contains the cpu identifier from the load script.
 *
 * CPUs which are not present are identified as -1
 * 'end' is identified as -1
 */
int convert_cpuchar_to_id(const char *cpustr, const int big, const int little)
{
	int tmp_cpu = -1;
	int search_cpu = 0;
	int len = strlen(cpustr);

	if(len == 1)
	{
		/* single characters, handle separately */
		char cpuchar = *cpustr;
		if(cpuchar >= '0' && cpuchar <='9')
		{
			tmp_cpu = cpuchar - '0';
		} else if (cpuchar == 'b' || cpuchar == 'B') {
			tmp_cpu = big;
		} else if(cpuchar == 'l' || cpuchar == 'L') {
			tmp_cpu = little;
		} else if(cpuchar == 'e' || cpuchar == 'E') {
			return -1;
		}
	} else {
		/* look for 'end' first */
		if((len == 3) && (tolower(cpustr[0])=='e') &&
				(tolower(cpustr[1])=='n') &&
				(tolower(cpustr[2])=='d'))
			return -1;
		/* otherwise, consider it as a number */
		tmp_cpu = strtol(cpustr, NULL, 10);
	}
	/* validate that the desired CPU exists in our calibration data */
	while(calibration_data[search_cpu].calibration_points)
	{
		if(search_cpu == tmp_cpu)
		{
			return tmp_cpu;
		}
		search_cpu++;
	}
	printf("ERROR: Asked for calibration data for CPU%d but it didn't exist.\n", tmp_cpu);
	return -1;
}

/*
 * For the given CPU (and with calibration data loaded), interpolate between the
 * existing calibrated points to identify the number of iterations of the busy loop
 * which should provide the desired percentage loading.
 */
int get_loop_amount(int cpu, int percent)
{
	struct loops_percent *data = calibration_data[cpu].data;
	int num_pct_points = (calibration_data[cpu].calibration_points-1);
	int i,low,high,lowp,highp;

	/* special case end of list */
	if(cpu < 0)
		return -1;

	/* check for special case percent bigger than max calibrated percent */
	if(percent > data[num_pct_points].percent)
	{
		printf("WARNING: %d%% activity is outside calibration data. Using value for %d%%.\n", percent, data[num_pct_points].percent);
		return data[num_pct_points].loops;
	}

	if(percent < data[0].percent)
	{
		/* We wish to create a load lower than our lowest calibrated
		 * amount. Any estimate is likely to be wildly inaccurate,
		 * so use the lowest one we have.
		 */
		printf("WARNING: %d%% activity is outside calibration data. Using value for %d%%.\n", percent, data[0].percent);
		return data[0].loops;
	}

	/* check which two percentages we are in between */
	for(i=0;i<num_pct_points;i++)
	{
		if(data[i].percent < percent)
		{
			low = data[i].loops;
			lowp = data[i].percent;
		} else if(data[i].percent == percent) {
			return data[i].loops; /* special case for exact match */
		} else if(data[i].percent > percent) {
			high = data[i].loops;
			highp = data[i].percent;
		}
	}
	/* if we get here,
	 *  low = last loop count below the target percent
	 *  lowp = last percent calibration point below the target percent
	 *  high = first loop count above the target percent
	 *  highp = first percent calibration point above target percent
	 */
	return low + ((high - low) * (percent - lowp)) / (highp - lowp);
}
