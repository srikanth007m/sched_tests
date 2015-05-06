#ifdef ANDROID
#ifndef ANDROID_SOURCE
#define DONOTHING
#endif
#endif

#ifndef DONOTHING
/*
Hi Ingo and all,

When I was executing massive interactive processes, I found that some of them
occupy CPU time and the others hardly run.

It seems that some of processes which occupy CPU time always has max effective
prio (default+5) and the others have max - 1. What happen here is...

 1. If there are moderate number of max interactive processes, they can be
    re-inserted into active queue without falling down its priority again and
    again.
 2. In this case, the others seldom run, and can't get max effective priority
    at next exhausting because scheduler considers them to sleep too long.
 3. Goto 1, OOPS!

Unfortunately I haven't been able to make the patch resolving this problem
yet. Any idea?

I also attach the test program which easily recreates this problem.

Test program flow:

  1. First process starts child proesses and wait for 5 minutes.
  2. Each child process executes "work 8 msec and sleep 1 msec" loop
     continuously.
  3. After 3 minits have passed, each child processes prints the # of loops
     which executed.

What expected:

  Each child processes execute nearly equal # of loops.

Test environment:

  - kernel:                 2.6.20(*1)
  - # of CPUs:                 1 or 2
  - # of child processes:  200 or 400
  - nice value:            0 or 20(*2)

*1) I confirmed that 2.6.21-rc5 has no change regarding this problem.
*2) If a process have nice 20, scheduler never regards it as interactive.

Test results:

-----------+----------------+------+------------------------------------
 # of CPUs | # of processes | nice |              result
-----------+----------------+------+------------------------------------
           |                |   20 | looks good
   1(i386) |                +------+------------------------------------
           |                |    0 | 4 processes occupy 98% of CPU time
-----------+            200 +------+------------------------------------
           |                |   20 | looks good
           |                +------+------------------------------------
           |                |    0 | 8 processes occupy 72% of CPU time
   2(ia64) +----------------+------+------------------------------------
           |            400 |   20 | looks good
           |                +------+------------------------------------
           |                |    0 | 8 processes occupy 98% of CPU time
-----------+----------------+------+------------------------------------

FYI. 2.6.21-rc3-mm1 (enabling RSDL scheduler) works fine in the all casees :-)

Thanks,

Satoru

-------------------------------------------------------------------------------
*/
/*
 * massive_intr - run @nproc interactive processes and print the number of
 *		  loops(*1) each process executes in @runtime secs.
 *
 *		  *1) "work 8 msec and sleep 1msec" loop
 *
 *	Usage:  massive_intr <nproc> <runtime>
 *
 *		 @nproc:   number of processes
 *		 @runtime: execute time[sec]
 *
 *	ex) If you want to run 300 processes for 5 mins, issue the
 *	    command as follows:
 *
 *		$ massive_intr 300 300
 *
 
A
A
A
*	How to build:
 *
 *		cc -o massive_intr massive_intr.c -lrt
 *
 *
 *  Copyright (C) 2007  Satoru Takeuchi <takeuchi_satoru@jp.fujitsu.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * Integrated in test framework by:
 * Signed-off-by: Naresh Kamboju <olivier.cozette@arm.com>
 * Ported to Android by:
 * Signed-off-by: Naresh Kamboju <naresh.kamboju@linaro.org>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#ifdef ANDROID
extern int ashmem_create_region(const char *name, size_t size);
extern int ashmem_set_prot_region(int fd, int prot);
extern int ashmem_pin_region(int fd, size_t offset, size_t len);
extern int ashmem_unpin_region(int fd, size_t offset, size_t len);
extern int ashmem_get_size_region(int fd);
#endif

#define WORK_MSECS	8
#define SLEEP_MSECS	1

#define MAX_PROC	1024
#define SAMPLE_COUNT	1000000000
#define USECS_PER_SEC	1000000
#define USECS_PER_MSEC	1000
#define NSECS_PER_MSEC	1000000
#define SHMEMSIZE	4096

static const char *shmname = "/sched_interactive_shmem";
static void *shmem;
static sem_t *printsem;
static int nproc = 0;
static int runtime = 0;
static int fd;
static time_t *first;
static pid_t pid[MAX_PROC];
static int return_code;

static void test_warn(char * name)
{
	printf("%s\n", name);
}

static void test_err(int code, char * name)
{
	printf("%s\n", name);
	exit(code);
}

#define err(a,b) test_err(a,b)
#define warn(a) test_warn(a)

static void cleanup_resources(void)
{
	if (sem_destroy(printsem) < 0)
		printf("sem_destroy() failed");
	if (munmap(shmem, SHMEMSIZE) < 0)
		printf("munmap() failed");
	if (close(fd) < 0)
		printf("close() failed");
}	

static void abnormal_exit(void)
{
	if (kill(getppid(), SIGUSR2) < 0)
	{
		printf("kill() failed");
		exit(EXIT_FAILURE);
	}
}

static void sighandler(int signo)
{
}

static void sighandler2(int signo)
{
	return_code = EXIT_FAILURE;
}

static void loopfnc(int nloop)
{
	int i;
	for (i = 0; i < nloop; i++)
		;
}

static int loop_per_msec(void)
{
	struct timeval tv[2];
	int before, after;

	if (gettimeofday(&tv[0], NULL) < 0)
		return -1;
	loopfnc(SAMPLE_COUNT);
	if (gettimeofday(&tv[1], NULL) < 0)
		return -1;
	before = tv[0].tv_sec*USECS_PER_SEC+tv[0].tv_usec;
	after = tv[1].tv_sec*USECS_PER_SEC+tv[1].tv_usec;

	return SAMPLE_COUNT/(after - before)*USECS_PER_MSEC;
}

static void *test_job(void *arg)
{
	int l = (int)(long)arg;
	int count = 0;
	time_t current;
	sigset_t sigset;
	struct sigaction sa;
	struct timespec ts = { 0, NSECS_PER_MSEC*SLEEP_MSECS};

	usleep(200000);
	sa.sa_handler = sighandler;
	if (sigemptyset(&sa.sa_mask) < 0) {
		warn("sigemptyset() failed");
		abnormal_exit();
	}
	sa.sa_flags = 0;
	if (sigaction(SIGUSR1, &sa, NULL) < 0) {
		warn("sigaction() failed");
		abnormal_exit();
	}
	if (sigemptyset(&sigset) < 0) {
		warn("sigfillset() failed");
		abnormal_exit();
	}
	sigsuspend(&sigset);
	if (errno != EINTR) {
		warn("sigsuspend() failed");
		abnormal_exit();
	}
	/* main loop */
	do {
		loopfnc(WORK_MSECS*l);
		if (nanosleep(&ts, NULL) < 0) {
			warn("nanosleep() failed");
			abnormal_exit();
		}
		count++;
		if (time(&current) == -1) {
			warn("time() failed");
			abnormal_exit();
		}
	} while (difftime(current, *first) < runtime);

	if (sem_wait(printsem) < 0) {
		warn("sem_wait() failed");
		abnormal_exit();
	}
	printf("%06d\t%08d\n", getpid(), count);
	if (sem_post(printsem) < 0) {
		warn("sem_post() failed");
		abnormal_exit();
	}
	exit(EXIT_SUCCESS);
}

static void usage(void)
{
	fprintf(stderr,
		"Usage : massive_intr <nproc> <runtime>\n"
		"\t\tnproc  : number of processes\n"
		"\t\truntime   : execute time[sec]\n");
	exit(EXIT_FAILURE);
}

int main_massive_intr(int argc, char **argv)
{
	int i, j;
	int status;
	sigset_t sigset;
	struct sigaction sa;
	int c;

/*	if (argc != 3)
		usage();*/
	
	if (argc >= 3)
		nproc = strtol(argv[1], NULL, 10);
	else
		nproc = 5;
	if (nproc < 1 || nproc > MAX_PROC)
		err(EXIT_FAILURE, "invalid multinum");

	if (argc >= 3)
		runtime = strtol(argv[2], NULL, 10);
	else
		runtime = 60;
	if (runtime <= 0)
		err(EXIT_FAILURE, "invalid runtime");

	sa.sa_handler = sighandler2;
	if (sigemptyset(&sa.sa_mask) < 0)
		err(EXIT_FAILURE, "sigemptyset() failed");
	sa.sa_flags = 0;
	if (sigaction(SIGUSR2, &sa, NULL) < 0)
		err(EXIT_FAILURE, "sigaction() failed");
	if (sigemptyset(&sigset) < 0)
		err(EXIT_FAILURE, "sigemptyset() failed");
	if (sigaddset(&sigset, SIGUSR1) < 0)
		err(EXIT_FAILURE, "sigaddset() failed");
	if (sigaddset(&sigset, SIGUSR2) < 0)
		err(EXIT_FAILURE, "sigaddset() failed");
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0)
		err(EXIT_FAILURE, "sigprocmask() failed");

	/* setup shared memory */
#ifdef ANDROID
	if ((fd = ashmem_create_region(shmname, SHMEMSIZE)) < 0) 
		err(EXIT_FAILURE, "ashmem_create_region() failed");
#else
	if ((fd = shm_open(shmname, O_CREAT | O_RDWR, 0644)) < 0)
		err(EXIT_FAILURE, "shm_open() failed");
	if (shm_unlink(shmname) < 0) {
		warn("shm_unlink() failed");
		goto err_close;
	}
	if (ftruncate(fd, SHMEMSIZE) < 0) {
		warn("ftruncate() failed");
		goto err_close;
	}
#endif
	shmem = mmap(NULL, SHMEMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shmem == (void *)-1) {
		warn("mmap() failed");
		goto err_unmap;
	}
	printsem = shmem;
	first = shmem + sizeof(*printsem);
	
	/* initialize semaphore */
	if ((sem_init(printsem, 1, 1)) < 0) {
		warn("sem_init() failed");
		goto err_unmap;
	}

	if ((c = loop_per_msec()) < 0) {
		fprintf(stderr, "loop_per_msec() failed\n");
		goto err_sem;
	}

	for (i = 0; i < nproc; i++) {
		pid[i] = fork();
		if (pid[i] == -1) {
			warn("fork() failed\n");
			for (j = 0; j < i; j++)
				if (kill(pid[j], SIGKILL) < 0)
					warn("kill() failed");
			goto err_sem;
		}
		if (pid[i] == 0)
			test_job((void *)(long)c);
	}

	if (sigemptyset(&sigset) < 0) {
		warn("sigemptyset() failed");
		goto err_proc;
	}
	if (sigaddset(&sigset, SIGUSR2) < 0) {
		warn("sigaddset() failed");
		goto err_proc;
	}
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		warn("sigprocmask() failed");
		goto err_proc;
	}
	if (time(first) < 0) {
		warn("time() failed");
		goto err_proc;
	}
	if ((kill(0, SIGUSR1)) == -1) {
		warn("kill() failed");
		goto err_proc;
	}
	for (i = 0; i < nproc; i++) {
		if (wait(&status) < 0) {
			warn("wait() failed");
			goto err_proc;
		}
	}
	cleanup_resources();
	exit(return_code);
 err_proc:
	for (i = 0; i < nproc; i++)
		if (kill(pid[i], SIGKILL) < 0)
			if (errno != ESRCH)
				warn("kill() failed");
 err_sem:
	if (sem_destroy(printsem) < 0)
		warn("sem_destroy() failed");
 err_unmap:
	if (munmap(shmem, SHMEMSIZE) < 0)
		warn("munmap() failed");
 err_close:
	if (close(fd) < 0)
		warn("close() failed");	
	exit(EXIT_FAILURE);
}
#else
int main_massive_intr(int argc, char **argv)
{
  return 0;
}
#endif
