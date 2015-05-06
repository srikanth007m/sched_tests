/*
 * this test is downloaded from
 * http://people.redhat.com/mingo/cfs-scheduler/tools
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <linux/unistd.h>
#include "test.h"

#define TV_2_LONG(tv) (tv.tv_sec*1E6+tv.tv_usec)
#define GET_TIME() \
	({ struct timeval __tv; gettimeofday(&__tv,NULL); TV_2_LONG(__tv); })

#define LOOPS 500000
#define TEST_SECONDS 60

int main_pipe_test(int argc, char ** argv)
{
	unsigned long long start, t0, t1;
	int fd1[2], fd2[2];
	int m = 0, i;
	long long nb = 0;

	pipe(fd1);
	pipe(fd2);
	start = GET_TIME();

	if (!fork()) {
		for (;;) {
			t0 = GET_TIME();
			for (i = 0; i < LOOPS; i++) {
				read(fd1[0], &m, sizeof(int));
				m = 2;
                                if (i == LOOPS - 1)
                                {
                                        m = 3;
                        t1 = GET_TIME();
                        printf("%.2f usecs/loop.\n",
                                (double)(t1-t0)/(double)LOOPS);
                                        if ((double)(t1-t0)/(double)LOOPS > 110)
                                                m = 4;
                                }
				write(fd2[1], &m, sizeof(int));
			}
			return 0;
                }
        } else {
                for (;;) {
                                m = 1;
                        nb++;
                                write(fd1[1], &m, sizeof(int));
                                read(fd2[0], &m, sizeof(int));
                        if (m == 3)
                        {
                                if (nb < LOOPS - 1)
                                        return 1;
                                return 0;
                        }
                        if (m == 4)
                                return 1;
			}
	}

	return 0;
}
