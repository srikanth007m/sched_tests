/*
 * original idea from lwn.net
 * 15 October 2012 changed by Olivier Cozette olivier.cozette@arm.com
 *	- Remove infinite loop
 *      - wait for end of the threads.
 *      - Check right interleave between task by checking a pipe buffer
 *          instead of reading stdout.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include "./test.h"
#include <sys/types.h>
#include <sys/wait.h>

#define LOOPS 80000
int pipes[2];

static void *proc_thread(void *arg)
{
	pthread_exit(0);
        return NULL;
}

void proc(char id)
{
	pthread_t t;
	int ret, j;
	int cnt = 0;

	for (j = 0; j < LOOPS; j++) {
		if (write(pipes[1], &id, 1) < 0)
			exit(1);
		ret = pthread_create(&t, 0, proc_thread, 0);
		if (ret) {
			printf("\nproc(%d) pthread_create() failed with %d!\n",
			       id, ret);
			exit(1);
		}
		if (write(pipes[1], &id, 1) < 0)
			exit(1);
		ret = pthread_join(t, 0);
		if (ret) {
			printf("proc(%d) pthread_join() failed with %d!\n",
			       id, ret);
			return;
		}
		if (write(pipes[1], &id, 1) < 0)
			exit(1);
		if (++cnt > 100) {
			cnt = 0;
		}
	}
}

int main_onetest(int argc, char **argv)
{
	char buffer[LOOPS * 3 * 2];
	int i, ret;
	int nb1 = 0;
	int nb2 = 0;
	int last_nb1 = -1;
	int last_nb2 = -1;
	int status;
	int received = 0;
	pid_t pids[2];
	if (pipe(pipes) == -1)
		return 1;
	pids[0] = fork();
	if (pids[0] == 0)
	{
		proc(2);
		return 0;
	}
	pids[1] = fork();
	if (pids[1] == 0)
	{
		proc(1);
	return 0;
	}

	while (received < LOOPS * 3 * 2)
	{
		ret = read(pipes[0], &buffer[received], LOOPS * 3 * 2 - received);
		if (ret >= 0)
			received += ret;
		sleep(1); /* read pipe to avoid any full pipe problen */
	}

	for (i = 0; i < 2; i++)
	{
		while (waitpid(pids[i], &status, 0) < 0)
			;
		if (!WIFEXITED(status))
		{
			printf("fork %d pid %d status %d (FAILED)\n", i, pids[i], status);
			return 1;
		}
	}


	/* checking if proc(1) was not scheduled too often */
	for (i = 0; i < LOOPS * 3 * 2; i++)
	{
		if (buffer[i] == 1)
		{
			last_nb1 = i;
			nb1++;
		}
		else if (buffer[i] == 2)
		{
			last_nb2 = i;
			nb2++;
		}
		else 
		{
			printf("invalid number in buffer[%d]=%d (FAILED)\n", i, buffer[i]);
			return 1;
		}

		if ((nb1 < LOOPS * 3) && (last_nb1 - i > 10))
		{
			printf("'1' was not scheduled since %d and %d, this is too long (FAILED)\n", last_nb1, i);
			return 1;
		}
		if ((nb2 < LOOPS * 3) && (last_nb2 - i > 10))
		{
			printf("'2' was not scheduled since %d and %d, this is too long (FAILED)\n", last_nb1, i);
			return 1;
		}			
	}
	return 0;
}
