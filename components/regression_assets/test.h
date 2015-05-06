/*
 * test.c - framework to integrate scheduler test
 *
 * Created by:  Olivier Cozette, July 2012
 * Copyright:   (C) 2012  ARM Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TEST_H
#define _TEST_H
#define TEST_MAX 32
#define TEST_TIMEOUT 120
#include <stdbool.h>
extern int main_chew(int argc, char **argv);
extern int main_fiftyp(int argc, char **argv);
extern int main_hackbench(int argc, char **argv);
extern int main_irman2(int argc, char **argv);
extern int main_massive_intr(int argc, char **argv);
extern int main_onetest(int argc, char **argv);
extern int main_starve(int argc, char **argv);
extern int main_tenp(int argc, char **argv);
extern int main_testcase(int argc, char **argv);
extern int main_thud2(int argc, char **argv);
extern int main_thud(int argc, char **argv);
extern int main_migrate(int argc, char ** argv);
extern int main_hmp_migration(int argc, char ** argv);
extern int main_affinity(int argc, char ** argv);
extern int main_cpuhog(int argc, char ** argv);
extern int main_pipe_test(int argc, char ** argv);
extern int main_pipe_test_1m(int argc, char ** argv);
extern int main_pth_str01(int argc, char ** argv);
extern int main_pth_str02(int argc, char ** argv);
extern int main_pth_str03(int argc, char ** argv);

/* helper */
extern int test_children_fork(void);
extern int test_children_gc(bool force); /* return 0 if success, 1 on error */
#endif
