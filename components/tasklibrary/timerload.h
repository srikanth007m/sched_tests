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

/* timerload - header for all of the load generator */

/*****************
 * configuration *
 *****************/

/* generic timer loop configuration */
/*
 * MILLISECONDS_PER_LOOP controls the number of
 * milliseconds apart that the alarm fires subject to system
 * resolution.
 *
 * Minimum value is 1.
 * When >1, times used in load scripts are scaled to match.
 */
#define MILLISECONDS_PER_LOOP 5

/* calibration configuration */
/*
 * SAMPLE_TIME_IN_MILLISECONDS
 * The amount of time we sample for in each calibration run
 */
#define SAMPLE_TIME_IN_MILLISECONDS 1000

/*
 * FAILED_CALIBRATION_COUNT
 * During calibration, this controls how many times we will
 * fail to find a good answer before giving up and using the
 * most accurate found so far.
 * This must be at least 2, since the first try is always
 * either a guess or the previous value.
 */
#define FAILED_CALIBRATION_COUNT 40
#if FAILED_CALIBRATION_COUNT < 2
#error "Must calibrate at least twice"
#endif
/*
 * MATCH_THRESHOLD_TIMES_TEN
 * This is the size of the percentage target in 0.1% increments.
 *
 */
#define MATCH_THRESHOLD_TIMES_TEN 5

/*
 * MICROSECOND_BINNING_AMOUNT
 * size of bins for histogram
 */
#define MICROSECOND_BINNING_AMOUNT 4

/*
 * PERCENT_SAMPLE_INCLUDES
 * the percentage of samples we consider when calculating
 * mean utilisation time
 */
#define PERCENT_SAMPLE_INCLUDES 80


/*************************************
 * derived configuration definitions *
 * do not change                     *
 *************************************/
/*
 * MICROSECONDS_PER_LOOP
 * The APIs use microseconds.
 */
#define MICROSECONDS_PER_LOOP (1000 * MILLISECONDS_PER_LOOP)
/*
 * MAX_NUM_RESULTS
 * The number of samples we will accumulate in each iteration
 */
#define MAX_NUM_RESULTS (SAMPLE_TIME_IN_MILLISECONDS / MILLISECONDS_PER_LOOP)


/** global vars **/
/* timer calibration data */
struct loops_percent {
	int percent;
	int loops;
};

struct cpu_data {
	int cpu;
	int calibration_points;
	struct loops_percent *data;
};

struct raw_loadseq_element
{
  int time;
  int loops;
  int priority;
};

extern char *calibration_file_name;
extern char calibfile_buffer[];
extern struct cpu_data *calibration_data;
extern int calibration_num_cpus;
extern int loadseq_to_run;
extern struct raw_loadseq_element *raw_loadseq_array;
extern volatile int busy_loops;
extern volatile int iterations_performed;
extern volatile int result_counter;



/** prototypes **/
/* timer load functions */
void waste_time(int loops);

/* utility functions */
int get_governor(int cpu, char* governor, int governor_len);
int set_governor(int cpu, char* governor);
void get_big_little_cpuids(int *big, int *little);
int convert_cpuchar_to_id(const char *cpustr, const int big, const int little);
int get_loop_amount(int cpu, int percent);

/* load sequence generator functions */
void build_loadseq(char *data);
void build_loadseq_file(char *data);
void run_loadseq(void);

/* calibration functions */
int calibrate_load(int percent, int start_loop_count);
int read_calibration();
void do_calibration();

/* text */
extern const char help_text[];
extern const char help_desc[];
extern const char calibrate_desc[];
extern const char calibfile_desc[];
extern const char display_desc[];
extern const char loadseq_desc[];
extern const char loadseqfile_desc[];
