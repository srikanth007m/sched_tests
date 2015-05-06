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

#include <sys/ptrace.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/user.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <asm/ptrace.h>

/*
 * Repeat the test so that we can make sure we run at least through one
 * powerdown scenario.
 */
static unsigned int repeat = 100;

/* Mask of bytes to trigger on. */
static unsigned int byte_address_select = 3;

/* Enum describing the different types of ARM hardware break-/watch-points. */
typedef enum {
	arm_hwbp_break = 0,
	/* ... */
} arm_hwbp_type;

static pid_t child_pid;

/*
 * Ensures the child and parent are always "talking" about
 * the same test sequence. (ie: that we haven't forgotten
 * to call check_trapped() somewhere).
 */
static int nr_tests;

/* Dummy functions to test execution accesses. */
static void dummy_func(void) { }
static void dummy_func1(void) { }
static void dummy_func2(void) { }
static void dummy_func3(void) { }

static void (*dummy_funcs[])(void) = {
	dummy_func,
	dummy_func1,
	dummy_func2,
	dummy_func3,
};

static int trapped;

static void check_trapped(void)
{
	/*
	 * If we haven't trapped, wake up the parent
	 * so that it notices the failure.
	 */
	if (!trapped)
		kill(getpid(), SIGUSR1);
	trapped = 0;

	nr_tests++;
}

static void trigger_tests(void)
{
	int i, ret;

	ret = ptrace(PTRACE_TRACEME, 0, NULL, 0);
	if (ret) {
		perror("Can't be traced?\n");
		return;
	}

	/* Wake up father so that it sets up the first test. */
	kill(getpid(), SIGUSR1);

	for (i = 0; i < 4*repeat; i++) {
		dummy_funcs[i%4]();
		check_trapped();
	}

	kill(getpid(), SIGUSR1);
}

static void check_success(const char *msg)
{
	const char *msg2;
	int child_nr_tests;
	int status;

	/* Wait for the child to SIGTRAP. */
	wait(&status);

	msg2 = "Failed";

	if (WSTOPSIG(status) == SIGTRAP) {
		child_nr_tests = ptrace(PTRACE_PEEKDATA, child_pid,
					&nr_tests, 0);
		if (child_nr_tests == nr_tests)
			msg2 = "Ok";
		if (ptrace(PTRACE_POKEDATA, child_pid, &trapped, 1)) {
			perror("Can't poke\n");
			exit(-1);
		}
	}

	nr_tests++;

	printf("%s [%s]\n", msg, msg2);
}

static void launch_arm_hw_breakpoints(char *buf)
{
	int i;
	unsigned int ctrl;

	/* From arm_hwbp_control_initialize() gdb/gdbserver/linux-arm-low.c. */
	ctrl = (byte_address_select << 5) | (arm_hwbp_break << 3) | (3 << 1);

	for (i = 0; i < 4*repeat; i++) {
		if (ptrace(PTRACE_SETHBPREGS, child_pid, 1 * ((i%4 << 1) + 1),
					&dummy_funcs[i%4])) {
			perror("Unexpected error setting breakpoint addr\n");
			exit(-1);
		}

		ctrl |= 0x01; /* Enable hw breakpoint. */
		if (ptrace(PTRACE_SETHBPREGS, child_pid, 1 * ((i%4 << 1) + 2),
					&ctrl)) {
			perror("Unexpected error enabling breakpoint\n");
			exit(1);
		}

		ptrace(PTRACE_CONT, child_pid, NULL, 0);
		sprintf(buf, "Test ARM hw breakpoint at addr=%p",
				dummy_funcs[i%4]);
		check_success(buf);

		ctrl &= ~0x01; /* Disable hw breakpoint. */
		if (ptrace(PTRACE_SETHBPREGS, child_pid, 1 * ((i%4 << 1) + 2),
					&ctrl)) {
			perror("Unexpected error disabling breakpoint\n");
			exit(1);
		}
	}
}

/* Set the breakpoints and check the child successfully trigger them. */
static void launch_tests(void)
{
	char buf[1024];

	launch_arm_hw_breakpoints(buf);

	ptrace(PTRACE_CONT, child_pid, NULL, 0);
}

int main(int argc, char **argv)
{
	pid_t pid;

	pid = fork();
	if (!pid) {
		trigger_tests();
		return 0;
	}

	child_pid = pid;

	wait(NULL);

	launch_tests();

	wait(NULL);

	/* Return error in case not all hw breakpoints were set successfully. */
	if (nr_tests != 4*repeat) {
		fprintf(stderr, "%s failed! (#hwbs/#hwbs set)=(%d/%d)\n",
				argv[0], 4*repeat, nr_tests);
		return -1;
	}

	return 0;
}
