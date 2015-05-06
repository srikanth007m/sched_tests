/*
 * original idea by Chris Friesen.  Thanks.
 * 3 August 2012 changed by Olivier Cozette olivier.cozette@arm.com
 *	- Run for 1 minutes
 *	- Expect we have enough compute capacity and processor 
 *           to never be preempted for more than 2ms
 *           if not return an error.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdbool.h>

#define THRESHOLD_USEC 2000ULL
#define THRESHOLD_MAX_USEC 65000ULL
#define SUM_BIGGER_THAN_THRESHOLD_USEC 190000
#define NB_BIGGER_THAN_THRESHOLD 5

static long t[64];
static unsigned long long sum[64];
static unsigned char my_log_data[256];
static unsigned long long sum_bigger_than_threshold = 0;
static unsigned long long nb_bigger_than_threshold = 0;
static unsigned long long max = 0;

static void my_log_init(void)
{
	int i;
	int nb = 1;
	int pow = 0;
	for (i = 0; i < 256; i++)
	{
		if (i > nb)
		{
			pow++;
			nb = nb * 2;
		}
		my_log_data[i] = pow;
	}
	for (i = 0; i < 64; i++)
	{
		t[i] = 0;
		sum[i] = 0;
	}
}

static int my_log(unsigned long long data)
{
	unsigned long long nb = 64;
	while (nb > 0)
	{
		unsigned long long temp = data >> ( nb - 8ULL);
		if (temp != 0)
			return my_log_data[temp] + (nb - 8);
		nb -= 8;
	}
	return 0;
}

static void print_tab(void)
{
	int i;
	unsigned long long sumsum = 0;
	unsigned long long nb = 1;
	for (i = 0; i < 31; i++)
	{
		printf("%ld %llu ", t[i], sum[i]);
		printf("# %lluus to %lluus \n", nb, nb*2);
		nb = nb * 2;
	}
}

static unsigned long long stamp()
{
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (unsigned long long) tv.tv_usec + ((unsigned long long) tv.tv_sec)*1000000;
}

bool loop_chew(unsigned long long run_seconds)
{
        unsigned long long thresh_ticks = THRESHOLD_USEC;
        unsigned long long cur, last, start, act, delta;
	int res;
        struct timespec ts;
        sched_rr_get_interval(0, &ts);
        start = stamp();
	last = start;
	do {
                cur = stamp();
                delta = cur-last;
		res = my_log(delta);
		t[res]++;
		sum[res] += delta;
		if (delta > max)
			max = delta;
                if (delta > thresh_ticks)
		{
			sum_bigger_than_threshold += delta;
			nb_bigger_than_threshold ++;
                }
                last = cur;
        } while (cur - start < run_seconds * 1000000);
        return true;
}

int main_chew(int argc, char ** argv)
{
	int i;
	int loop = 6;
	int time_seconds = 8;
	int sleep_seconds = 2;
	unsigned long long time_schedule_out;
	my_log_init();

	for (i = 0; i < loop; i++)
	{
		printf("#Loop %d/%d\n", i, loop);
		sleep(sleep_seconds);
		if (!loop_chew(time_seconds))
			return 1;
	}
	print_tab();
	sum_bigger_than_threshold /= time_seconds;
	nb_bigger_than_threshold /= time_seconds;
	printf("# %llu us of schedule out interval bigger than %llu ms per runnable second (%s)\n", 
		sum_bigger_than_threshold , THRESHOLD_USEC / 1000ULL,
		(sum_bigger_than_threshold > SUM_BIGGER_THAN_THRESHOLD_USEC)?"FAILED":"SUCCESS");
	printf("# %llu number of schedule out interval bigger than %llu ms per runnable second (%s)\n", 
		nb_bigger_than_threshold , THRESHOLD_USEC / 1000ULL,
		(nb_bigger_than_threshold > NB_BIGGER_THAN_THRESHOLD)?"FAILED":"SUCCESS");
	printf("# %llu maximum schedule out interval (%s)\n", 
		max,
		(max > THRESHOLD_MAX_USEC)?"FAILED":"SUCCESS");
	if (sum_bigger_than_threshold > SUM_BIGGER_THAN_THRESHOLD_USEC)
		return 1;
	if (max > THRESHOLD_MAX_USEC)
		return 1;
	if (nb_bigger_than_threshold > NB_BIGGER_THAN_THRESHOLD)
		return 1;
	return 0;
}

