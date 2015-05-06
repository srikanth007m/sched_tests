#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./test.h"


static int nbthreads = 0;
static int iterations = 0;
static volatile int run = 1;
static int iterations2 = 0;

#define TEST_SECONDS 6
#define DEFAULTBUFSIZE 1
static int globalpipe[2];

static int buflen = DEFAULTBUFSIZE;

static int thread_read[256];

static char *fname;


static unsigned long long get_nsecs(void)
{
	struct timespec myts;
	clock_gettime(CLOCK_REALTIME, &myts);
	return (myts.tv_sec * 1000000000ULL + myts.tv_nsec );
}

static void *thread_code(void *arg) {
        time_t lasttime = time(NULL);
        int jj = (int)(long)arg;
        int localiter = iterations/jj;
        int fd;

        char *buf;

        buf = alloca(buflen);
        if (buf == NULL) {
                printf("Failed to alloca!\n");
                exit(-1);
        }

        do {
                int i;
                time_t newtime;

                for(i=0; i < localiter && run; i++) {
                        int j;
                        double k = 1.0;
                        for(j=0; j < iterations2 && run; j++) {
                                k += j*k;
                                k = (1-k)*(1+k);
                                k = sqrt(k);
                        }
                        if (fd != -1) {
                                if (read(globalpipe[0], buf, buflen)!= buflen) {
                                        printf("%d, aborted read\n", jj);
                                }
				else
				{
					thread_read[(long)arg]++;
				}
                        }

                }
                sleep(1);
                newtime = time(NULL);
                printf("%d    delta = %ld (total %d)\n", (int)(long)arg, newtime-lasttime, thread_read[(long)arg]);
                lasttime = newtime;
        } while (run);
        return NULL;
}


int main_testcase(int argc, char **argv)
{
        int i;
	int nb = 0;
	unsigned long long time;
        pthread_t * mythreads;

        nbthreads = 16;
        iterations = 32;
        iterations2 = 32;
        buflen = DEFAULTBUFSIZE * 1;

        if (pipe(globalpipe) != 0) {
                return -2;
        }

        mythreads = (pthread_t *)calloc(nbthreads, sizeof(pthread_t));

        for(i=0; i < nbthreads; i++) {
                if (pthread_create(&mythreads[i], NULL, thread_code, (void *)(long)(i+1)) != 0) {
                        return -5;
                }
                printf("Started %d\n", i);
		thread_read[i] = 0;
        }


        void * res;

        char *buf;
        int localbuflen;

        localbuflen = buflen * nbthreads;

        buf = alloca(localbuflen);
        if (buf == NULL) {
                printf("Failed to alloca!\n");
                run = 0;
                exit(-1);
        }

        sleep(3);

	
	time = get_nsecs();
        do {
                write(globalpipe[1], buf, localbuflen);
		nb ++;
        } while (get_nsecs() - time <  TEST_SECONDS * 1000000000ULL);

	run = 0;

        for(i=0; i < nbthreads; i++) {
                printf("Waiting %d\n", i);
                pthread_join(mythreads[i], &res);
        }
        return 0;
}

