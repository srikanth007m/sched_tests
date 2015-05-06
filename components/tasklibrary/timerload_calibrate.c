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

/* timerload_calibrate - calibration controller for load generator */

#include <signal.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <sched.h>
#include <sys/sysinfo.h>

#include "timerload.h"

volatile int calibrate_attempts;
char default_calibration_file_name[]="./calib.txt";
char *calibration_file_name = default_calibration_file_name;
char calibfile_buffer[4096];

static int max_num_results=MAX_NUM_RESULTS;
volatile struct timestamps
{
	struct timeval start_time;
	struct timeval end_time;
}results[MAX_NUM_RESULTS];

static void reset_array()
{
	memset((void *)results, 0, (size_t)(sizeof(struct timestamps)*MAX_NUM_RESULTS));
}

/* The signal handler clears the flag and records the time intervals */
static void catch_alarm_calibrate (int sig)
{
  struct timeval time;
  if(result_counter < MAX_NUM_RESULTS){
	  gettimeofday(&time,0);
	  results[result_counter].start_time.tv_sec = time.tv_sec;
	  results[result_counter].start_time.tv_usec = time.tv_usec;
	  /* run the busy loop */
	  iterations_performed = busy_loops;
	  waste_time(busy_loops);

	  gettimeofday(&time,0);
	  results[result_counter].end_time.tv_sec = time.tv_sec;
	  results[result_counter++].end_time.tv_usec = time.tv_usec;
	  if(iterations_performed != busy_loops)
		  printf("WARNING: Calibration missed %d loops\n", busy_loops - iterations_performed);
  }

  if(calibrate_attempts)
	  calibrate_attempts--;
}

/*
 * run the busy loop until the required number of results have been collected
 */
static int get_results(int num_results)
{
	/* set start conditions */
	calibrate_attempts=num_results;
    result_counter=0;

	/* Establish a handler for SIGALRM signals. */
	if(signal(SIGALRM, catch_alarm_calibrate) == SIG_ERR)
		return -1;
	/* Set an alarm to go off in a little while. */
	ualarm(MICROSECONDS_PER_LOOP,MICROSECONDS_PER_LOOP);

	while(calibrate_attempts){
		pause();
	};

	/* cancel alarm */
	ualarm(0,0);

	return 0;
}

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.

   Taken from GNU libc manual at
   http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
*/

static int
timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

/* histogram of used time in busy loop execution
 *
 * array should be created of MAX_NUM_RESULTS elements
 * and cleared using reset_used_val_array before use.
 *
 * All functions using this array expect used=0 to terminate
 * the array
 *
 * A second array of pointers to these elements is used when
 * the histogram is sorted - the array elements are not moved
 */
struct used_histogram_element
{
	int used;
	int count;
};

static void reset_used_val_array(struct used_histogram_element *array, int count)
{
	/* clear histogram */
	memset(array, 0, sizeof(struct used_histogram_element) * MAX_NUM_RESULTS);
}
/*
 * add a specific used value to the histogram, incrementing its frequency count
 */
static void increment_used_val(struct used_histogram_element *array, int count, int orig_val)
{
	int i,next_free,set=0, used_val;
	/* strip bits from the bottom of used_val to provide binning */
	used_val = orig_val - (orig_val % MICROSECOND_BINNING_AMOUNT);
	used_val += 1; // make sure we can store '0' values

	for(i=0;i<count;i++)
	{
		if(array[i].used == 0)
		{
			next_free = i;
			break;
		}
		if(array[i].used == used_val)
		{
			array[i].count++;
			set++;
			break;
		}
	}
	if(!set)
	{
		array[next_free].used = used_val;
		array[next_free].count = 1;
	}
}

/*
 * sorting utility function
 *
 * insert a pointer to a histogram element at a given position, making space if necessary
 */
static void insert_at_pos(struct used_histogram_element **array, int max_elements, int index, struct used_histogram_element *el)
{
	if(index && array[index] != NULL)
	{
		int size_to_move = sizeof(struct used_histogram_element *) * (max_elements - index - 1);
		memmove(&array[index+1], &array[index], size_to_move);
	}
	array[index] = el;
}

/*
 * sort a histogram by frequency into a given array of pointers to elements
 */
static void sort_histogram_array(struct used_histogram_element **sorted_array, struct used_histogram_element *array, int max_elements)
{
	int i,j;
	/* generally not many elements, just use an insertion sort */
	for(i=0;i<max_elements;i++)
	{
		if(array[i].count == 0)
			break;

		for(j=0;j<max_elements;j++)
		{
			if(!sorted_array[j] || array[i].count >= sorted_array[j]->count)
			{
				insert_at_pos(sorted_array, max_elements, j, &array[i]);
				break;
			}
		}
	}
	for(i=0;i<max_elements;i++)
	{
		if(!sorted_array[i] || sorted_array[i]->count == 0)
			break;
	}
}

/*
 * Return the mean of used microseconds across the most-frequent 'percent_average_mean'% of the results
 *
 * i.e. if percent_average_mean is 50, get the mean of at least the most-frequent 50% of the data
 * If a bin contributes more than is needed to reach the desired percentage, the entire bin is counted
 *
 * Mean is calculated based upon the number of samples included in the result
 */
static int get_percent_average_mean(struct used_histogram_element *array, int max_elements, int percent_average_mean)
{
	int i, num_elements=0, sum_of_counted_elements=0, counted_elements=0, mean=0;
	struct used_histogram_element **sorted_histogram;

	sorted_histogram = malloc(sizeof(struct used_histogram_element*) * max_elements);
	memset(&sorted_histogram[0], 0, sizeof(struct used_histogram_element *) * max_elements);

	/* Algorithm:
	 * Out of all the recorded results in the array, collect from the most frequent downover until we have
	 * included 'percent_average_mean' % of the results.
	 *
	 * Take a mean of these results
	 */
	sort_histogram_array(sorted_histogram, array, max_elements);
	/* find out how many results we have */
	for(i=0;i<max_elements;i++)
	{
		if(!sorted_histogram[i] || !sorted_histogram[i]->count)
			break;
		num_elements += sorted_histogram[i]->count;
	}
	if(num_elements)
	{
		int min_elements_to_include;
		min_elements_to_include = (num_elements * 100) / percent_average_mean;

		i = 0;
		while(sorted_histogram[i] && counted_elements < min_elements_to_include)
		{
			counted_elements += sorted_histogram[i]->count;
			sum_of_counted_elements += ((sorted_histogram[i]->used-1) * sorted_histogram[i]->count);
			i++;
		}
		mean = sum_of_counted_elements / counted_elements;
		/* mean is now the mean number of microseconds active in the top percent of results */
		//printf("Mean: %d\n", mean);
	}
	free(sorted_histogram);
	return mean;
}

/*
 * get the average percentage utilisation multipled by 10 so we can measure
 * to 0.1% accuracy
 */
static int get_average_percent_utilised_byten(int num_entries)
{
	int i, used;
	struct used_histogram_element used_histogram[MAX_NUM_RESULTS];
	double util;

	/* clear out the result histogram */
	reset_used_val_array(used_histogram, MAX_NUM_RESULTS);

	for(i=0; i<num_entries-1;i++)
	{
		struct timeval time;
		timeval_subtract(&time, &results[i].end_time, &results[i].start_time);
		used = time.tv_usec;
		increment_used_val(used_histogram, MAX_NUM_RESULTS, used);
	}

	used = get_percent_average_mean(used_histogram, MAX_NUM_RESULTS, PERCENT_SAMPLE_INCLUDES);
	util = used * 100.0f / MICROSECONDS_PER_LOOP;

	return (util * 10.0f);
}

int calibrate_load(int percent, int start_loop_count)
{
	int calibrated=0;
	int tentimespercent=percent*10;
	int lastresult=0,repeats=0;
	int best_result_so_far=0, best_loop_so_far=0;
	int iterations = 0;
	/* we are aiming for 'percent' %busy */

	/* initial loop count is either provided or we make a guess */
	busy_loops=start_loop_count ? start_loop_count : 600 * percent * MILLISECONDS_PER_LOOP;
	do{
		int util=0; /* percentage utilisation, multiplied by 10 */
		int loops_per_tenth_percent, overshoot;

		reset_array();
		if(get_results(max_num_results))
			break;

		util=get_average_percent_utilised_byten(max_num_results);
		iterations++;
        /* algorithm
         *
         * We are looking for a percentage utilisation between
         * (target - MATCH_THRESHOLD_TIMES_TEN) and
         * (target + MATCH_THRESHOLD_TIMES_TEN)
         *
         * If it is not found, we scale the current number of loops
         * linearly based upon the current utilisation to obtain a
         * better estimate.
         *
         * If we cannot find an estimate within FAILED_CALIBRATION_COUNT
         * attempts, we use the closest one we found.
         */

        /* if there was no measurable utilisation, double the iterations */
		if(!util)
		{
			busy_loops <<= 1;
			continue;
		}

		/* keep track of the closest result obtained - will be used if we fail to
		 * get a close-enough match
		 */
        if(abs(util - tentimespercent) < abs(best_result_so_far - tentimespercent))
        {
        	best_result_so_far = util;
        	best_loop_so_far = busy_loops;
        }

        /* track how many times we repeat a result */
		if(util == lastresult)
			repeats++;
		else
		{
			lastresult=util;
			repeats=0;
		}

		/* if we repeat a result more than twice, use the last result */
		if(repeats>2)
		{
			printf("ERROR:Unable to reach %d%%. Got to %f with %d loops\n", percent, util/10.0f,busy_loops);
			return busy_loops;
		}

		/* first, how many loops are in 0.1% of the result? */
		loops_per_tenth_percent = busy_loops / util;
		/* work out how far away from the target we are */
		overshoot = util - tentimespercent;
		/* if we are within the target of the result, we have calibrated */
		if(abs(overshoot) <= MATCH_THRESHOLD_TIMES_TEN)
		{
			calibrated=1;
			break;
		}

		/* calculate the number of loops in the next iteration */
		busy_loops = busy_loops - (loops_per_tenth_percent * overshoot);
		printf("Target missed by %f%%: new busy_loops=%d\n", overshoot/10.0f, busy_loops);

		/* if we tried too many times and failed to calibrate, use the closest result so far */
		if(iterations > FAILED_CALIBRATION_COUNT)
		{
			busy_loops = best_loop_so_far;
			printf("Failed to reach a stable loop count. Using %d\n", busy_loops);
			calibrated=1;
		}
	}while(!calibrated);

	if(calibrated)
		return busy_loops;
	else
		return -1;
}

/* obtain calibration data for all present CPUs at 21 known calibration points.
 *
 * Points are 1%,5% ..(5% increments).. 95%,99%
 */
void do_calibration()
{
	int i,cpu,num_cpus=get_nprocs();
	int num_tests=21;
	int previous_result[100], last_result;
	pid_t my_pid=getpid();
	cpu_set_t cpuset;
	cpu_set_t original_cpuset;
	FILE *output;
	memset(previous_result, 0, sizeof(int) * 100);

	output=fopen(calibration_file_name,"w");
	if(!output)
		return;
	fprintf(output, "%d:%d\n",num_cpus, num_tests);
	/* take the original affinity mask of this task */
	sched_getaffinity(my_pid, sizeof(cpu_set_t), &original_cpuset);
	for(cpu=0; cpu<num_cpus; cpu++)
	{
		char cpu_governor[32];
		printf("On CPU%d\n", cpu);
		/*
		 * Try to set the CPU governor to performance to get reliable results
		 * in case DVFS is running
		 */
		if(get_governor(cpu,cpu_governor,31)>0 && set_governor(cpu,"performance")>0)
			fprintf(output, "%d\n",cpu);
		else
			fprintf(output, "%d*\n",cpu);

		__CPU_ZERO_S(sizeof(cpu_set_t), &cpuset);
		__CPU_SET_S(cpu, sizeof(cpu_set_t), &cpuset);

		/* place ourselves on the CPU we are calibrating for */
		sched_setaffinity(my_pid, sizeof(cpu_set_t), &cpuset);

		/* aim for 1% */
		previous_result[1]=calibrate_load(1,previous_result[1]);
		last_result = previous_result[1];
		printf("Need %d loops to hit %d percent\n\n", previous_result[1],1);
		fprintf(output,"%d:%d\n",1, previous_result[1]);
		fflush(output);

		/* compute the 5% series */
		for(i=5;i<100;i+=5){
			previous_result[i]=calibrate_load(i, (previous_result[i] != 0 ? previous_result[i]:last_result) );
			last_result = previous_result[i];
			printf("Need %d loops to hit %d percent\n\n", previous_result[i],i);
			fprintf(output,"%d:%d\n",i, previous_result[i]);
			fflush(output);
		}

		/* aim for 99% */
		previous_result[99]=calibrate_load(99,previous_result[99]);
		printf("Need %d loops to hit %d percent\n\n", previous_result[99],99);
		fprintf(output,"%d:%d\n",99, previous_result[99]);
		fflush(output);

		/* restore the governor */
		set_governor(cpu,cpu_governor);
	}
	/* restore the original CPU affinity mask */
	sched_setaffinity(my_pid, sizeof(cpu_set_t), &original_cpuset);
	fclose(output);
}

/*
 * clean up calibration data
 */
void free_data_structs()
{
	if(calibration_data)
	{
		int i;
		for(i=0;i<calibration_num_cpus;i++)
		{
			if(calibration_data[i].data)
				free(calibration_data[i].data);
		}
		free(calibration_data);
	}
	calibration_data = 0;
	calibration_num_cpus = 0;
}

/*
 * create storage for calibration data
 */
void alloc_data_structs(int num_cpus, int num_points)
{
	int i;
	int failed=0;
	/* alloc one extra for termination */
	calibration_data = malloc((num_cpus+1) * sizeof(struct cpu_data));
	if(calibration_data)
	{
		calibration_num_cpus = num_cpus;
		memset(calibration_data, 0, (num_cpus+1) * sizeof(struct cpu_data));
		for(i=0; i < num_cpus; i++)
		{
			calibration_data[i].data = malloc(num_points * sizeof(struct loops_percent));
			if(calibration_data[i].data)
				memset(calibration_data[i].data, 0, num_points * sizeof(struct loops_percent));
			else
				failed=1;
		}
		/* terminate list */
		calibration_data[num_cpus].calibration_points = 0;
		if(failed)
			free_data_structs();
	}
}


/********************************
 * calibration data load parser *
 ********************************/

struct parser_context
{
	int num_cpus;
	int num_points_per_cpu;
	int curr_cpu;
	int curr_point;
};

int handle_line(char *line)
{
	static struct parser_context context;
	int returnval=1;
	char *ptr;
	int len = strlen(line);

	if(!calibration_data)
	{
		int num_cpus=0;
		int num_points=0;
		/* first line contains all needed to set up context */
		ptr = strtok(line, ":");
		if(!ptr)
			return 0;
		num_cpus = strtol(ptr,0,10);
		ptr = strtok(0,":");
		if(!ptr)
			return 0;
		num_points = strtol(ptr,0,10);

		alloc_data_structs(num_cpus,num_points);
		if(!calibration_data)
			return 0;
		context.num_cpus = num_cpus;
		context.num_points_per_cpu = num_points;
		context.curr_cpu = -1;
		context.curr_point = 0;
	} else {
		if(context.curr_cpu == -1)
		{
			int this_cpu;
			/* the line we have ought to contain a CPU definition line */
			if(line[len-2]=='*')
				returnval=2;
			this_cpu = strtol(line,0,10);
			if(this_cpu >= context.num_cpus)
				goto fatal;
			context.curr_cpu = this_cpu;
			context.curr_point = 0;
			calibration_data[this_cpu].cpu = this_cpu;
			calibration_data[this_cpu].calibration_points = context.num_points_per_cpu;
		}
		else
		{
			/* the line we have ought to contain a percent/loops point line for the current CPU */
			int percent,loops;
			ptr = strtok(line, ":");
			if(!ptr)
				goto fatal;
			percent = strtol(ptr,0,10);
			ptr = strtok(0,":");
			if(!ptr)
				goto fatal;
			loops = strtol(ptr,0,10);
			calibration_data[context.curr_cpu].data[context.curr_point].percent = percent;
			calibration_data[context.curr_cpu].data[context.curr_point].loops = loops;
			context.curr_point++;
			if(context.curr_point >= context.num_points_per_cpu)
				context.curr_cpu = -1;
		}

	}
	goto end;
fatal:
	free_data_structs();
	returnval=0;
end:
	return returnval;
}

/*
 * pass the calibration data from the file to the line-by-line parser
 */
int read_calibration()
{
	FILE * input;
	char buffer[128];
	int lineresult, no_control=0;

	input = fopen(&calibration_file_name[0], "r");
	if(!input)
		return 0;

	calibration_data = NULL;

	while(fgets(buffer,128,input)){
		lineresult = handle_line(buffer);
		if(!lineresult)
			return 0;
		if(lineresult == 2)
			no_control=1;
	};
	if(no_control!=0)
		printf("No Governor Control during calibration.\n");

	fclose(input);
	return (calibration_data != NULL);
}
