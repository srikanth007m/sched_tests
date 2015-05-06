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

#ifndef _OS_H
#define _OS_H
#include <stdbool.h>

enum os_init_t
{
	OS_INIT_NOFREQUENCY,
	OS_INIT_ALL,
};

extern void os_init(enum os_init_t param);
extern void os_cleanup(void);
extern int os_cpu_nb(void);
extern double os_time(void);

/* nb = size of temperature
 * if size of temporature != os_cpu_nb(), it will fail
 */
extern bool os_temperature_get(double * temperature, int nb);

/* cpu = -1 means all cpus */
extern bool os_affinity_set(int cpu);


/* return -1 if problem happened */
extern long long os_frequency_get(void);

/* false if not successfull */
extern bool os_frequency_set(long long freq);

extern void os_frequency_available(const long long * * freqs, int * freqs_nb);

extern void os_sleep(double seconds);


/* create a child process detached from the caller
 * the parent get as return of this function the pid of the child
 * the child run the child() function and exit when the function is finished
 */
extern int os_child_create_deamon(void (*child)(int pid, void * data), void * data);


/* create a child process still attached to the caller
 * the parent get as return of this function the pid of the child
 * the child run the child() function and exit when the function is finished
 */
extern int os_child_create(void (*child)(int pid, void * data), void * data);


/* wait for a child created with os_child_create()
 * *exited = true if the child exited, false if it still running
 * *success = true if the child exited successfully, false otherwise
 * if not exited, *cpu cpu it was on when the call was done
 * This is a blocking call */
extern void os_child_check(int pid, bool * exited, bool * success, int * cpu);

extern bool os_nice_set(int pid, int nice);

/* give usage of this and all children create using os_child_create()
 */
extern bool os_usage_get(double * system, double * user);
#endif
