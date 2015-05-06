/*
 * 3 August 2012 Change by Olivier Cozette olivier.cozette@arm.com
 */
// gcc -O2 -o fiftyp fiftyp.c -lrt
// code from interbench.c

#define TEST_SECONDS 60
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
int forks= 10;
int runus,sleepus=7000;
static unsigned long loops_per_ms;
static void terminal_error(const char *name)
{
	fprintf(stderr, "\n");
	perror(name);
	exit (1);
}

static unsigned long long get_nsecs(struct timespec *myts)
{
	if (clock_gettime(CLOCK_REALTIME, myts))
		terminal_error("clock_gettime");
	return (myts->tv_sec * 1000000000ULL + myts->tv_nsec );
}

static void burn_loops(unsigned long loops)
{
	unsigned long i;

	/*
	 * We need some magic here to prevent the compiler from optimising
	 * this loop away. Otherwise trying to emulate a fixed cpu load
	 * with this loop will not work.
	 */
	for (i = 0 ; i < loops ; i++)
	     asm volatile("" : : : "memory");
}

/* Use this many usecs of cpu time */
static void burn_usecs(unsigned long usecs)
{
	unsigned long ms_loops;

	ms_loops = loops_per_ms / 1000 * usecs;
	burn_loops(ms_loops);
}

static void microsleep(unsigned long long usecs)
{
	struct timespec req, rem;

	rem.tv_sec = rem.tv_nsec = 0;

	req.tv_sec = usecs / 1000000;
	req.tv_nsec = (usecs - (req.tv_sec * 1000000)) * 1000;
continue_sleep:
	if ((nanosleep(&req, &rem)) == -1) {
		if (errno == EINTR) {
			if (rem.tv_sec || rem.tv_nsec) {
				req.tv_sec = rem.tv_sec;
				req.tv_nsec = rem.tv_nsec;
				goto continue_sleep;
			}
			goto out;
		}
		terminal_error("nanosleep");
	}
out:
	return;
}

/*
 * In an unoptimised loop we try to benchmark how many meaningless loops
 * per second we can perform on this hardware to fairly accurately
 * reproduce certain percentage cpu usage
 */
static void calibrate_loop(void)
{
	unsigned long long start_time, loops_per_msec, run_time = 0;
	unsigned long loops;
	struct timespec myts;

	loops_per_msec = 1000000;
redo:
	/* Calibrate to within 1% accuracy */
	while (run_time > 1010000 || run_time < 990000) {
		loops = loops_per_msec;
		start_time = get_nsecs(&myts);
		burn_loops(loops);
		run_time = get_nsecs(&myts) - start_time;
		loops_per_msec = (1000000 * loops_per_msec / run_time ? :
			loops_per_msec);
	}

	/* Rechecking after a pause increases reproducibility */
	sleep(1);
	loops = loops_per_msec;
	start_time = get_nsecs(&myts);
	burn_loops(loops);
	run_time = get_nsecs(&myts) - start_time;

	/* Tolerate 5% difference on checking */
	if (run_time > 1050000 || run_time < 950000)
		goto redo;
 loops_per_ms=loops_per_msec;
 sleep(1);
 start_time=get_nsecs(&myts);
 microsleep(sleepus);
 run_time=get_nsecs(&myts)-start_time;
 runus=run_time/1000;
}

int main_fiftyp(int argc, char **argv)
{
 struct timespec myts;
 int pids[256];
 unsigned long long time;
        int i;
        calibrate_loop();
        printf("starting %d forks\n",forks);
        for(i=1;i<forks;i++){
  pids[i] = fork() ;
  if(pids[i] < 0)
        return 1;
  if(pids[i] == 0)
  {
         time = get_nsecs(&myts);
         while (get_nsecs(&myts) - time < TEST_SECONDS * 1000000000ULL)
         {
                burn_usecs(runus);
                microsleep(sleepus);
        }
	return 0;
 }
}
 for(i=1;i<forks;i++){
	int status = 0;
	while (waitpid(pids[i], &status, 0) < 0)
		;
	if (!WIFEXITED(status))
	{
		printf("fork %d pid %d status %d (FAILED)\n", i, pids[i], status);
		return 1;
	}
        printf("fork %d pid %d status %d (SUCCESS)\n", i, pids[i], status);
 }
	return 0;
}
