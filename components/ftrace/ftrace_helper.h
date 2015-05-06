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

/* ftrace_helper - helper for common ftrace analysis library */

#ifndef _FTRACE_HELPER_H
#define _FTRACE_HELPER_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
extern bool ftrace_helper_cpu_info(int cpu, int * pid, long long * frequency);
extern bool ftrace_helper_task_info(int pid, int * cpu, double * sched_in, double * sched_out, bool * waiting, bool * running);
extern bool ftrace_helper_task_waiting(int pid, double * last_running, double * last_waiting);

/**
 * Read environmet variable. If "HH" equal "5,8,9,10"
 * ftrace_helper_getenv_array("HH", values, 6) will return 4 and value will be {5,8,9,10}.
 * CPU_FAST and CPU_SLOW are always defined but could be overiden.
 * "CPU_FAST=3,4 ./ftrace -l libmigration.so.1.0.0 -t myftrace" will overide CPU_FAST.
 */
extern int ftrace_helper_getenv_array(const char * name, double * values, int max);
extern int ftrace_helper_getenv_array_int(const char * name, int * values, int max);
extern void ftrace_helper_init(void);
void ftrace_helper_cleanup(void);


/**
 * Histogram mangement
 */
#define restrict
struct histogram;
extern struct histogram * histogram_create(int index_max, int step_max, double min, double step);
extern void histogram_destroy(struct histogram * hist);
extern void histogram_dump(struct histogram * hist, const char * filename);
extern void histogram_info(struct histogram * hist, int index, double * time, double * avg);
extern void histogram_add(struct histogram * restrict hist, int index, double time, double value);
#ifdef __cplusplus
}
#endif
#endif
