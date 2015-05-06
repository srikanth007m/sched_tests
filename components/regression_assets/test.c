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

#include <stdbool.h>
#include "./test.h"
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CHILDREN_MAX_PID 64
static struct
{
	int (*main[TEST_MAX])(int argc, char **argv);
	const char * name[TEST_MAX];
	int fd[2];
	int fd0;
	int fd1;
	int fd2;
} test;

static struct {
	int pid_nb;
	int pids[CHILDREN_MAX_PID];
	bool parent;
} test_children;

int test_max = 0;

static void test_children_init(void)
{
	test_children.parent = true;
}

int test_children_fork(void)
{
	int ret;
	if (test_children.pid_nb >= CHILDREN_MAX_PID)
		return -1;
	ret = fork();
	test_children.parent = true; /* parent of another children */
	if (ret == 0)
	{
		test_children.parent = false;
		test_children.pid_nb = 0;
		return ret;
	}
	test_children.pids[test_children.pid_nb] = ret;
	test_children.pid_nb ++;
	return ret;
}

int test_children_gc(bool force)
{
	if (!test_children.parent)
		return true;
	while (test_children.pid_nb > 0)
	{
		test_children.pid_nb--;
		while (test_children.pid_nb > 0)
		{
			int status;
			test_children.pid_nb --;
			if (force)
				kill(test_children.pids[test_children.pid_nb], SIGKILL);
			while (waitpid(test_children.pids[test_children.pid_nb], &status, 0) < 0)
	                        ;
			if (!WIFEXITED(status))
			{
				printf("fork %d pid %d status %d (FAILED)\n",
					test_children.pid_nb,
					test_children.pids[test_children.pid_nb],
					status);
				return 1;
			}
		}
	}
	return 0;
}

static bool test_run(const char * name, int argc, char **argv)
{
	int ret;
	bool success = false;
	bool ended = false;
	pid_t pid;
	int test_nb = 0;
	struct timeval timeout;
	timeout.tv_sec = TEST_TIMEOUT;
	timeout.tv_usec = 0;
	char * error = "";


	while ((test_nb < test_max) && (strcmp(name, test.name[test_nb]) != 0))
		test_nb++;
	if (test_nb >= test_max)
		return false;
	if (pipe(&test.fd[0]) < 0)
		return false;

	printf("Test %20s %s", name, (test.fd0 == -1)?"\n":"");
	fflush(stdout);

	if (test.fd1 >= 0)
	{
		FILE * fd = fopen("/proc/version", "r");
		char c;
		write(test.fd1, "--- Test name <", strlen("--- Test name <"));
		write(test.fd1, name, strlen(name));
		write(test.fd1, ">\n", strlen(">\n"));
		write(test.fd1, "--- kernel version <", strlen("--- kernel version <"));
		while (fscanf(fd, "%c", &c) > 0)
		{
			if  ((c == 10) || (c == 13) || (c == '>'))
				c = ' ';
			write(test.fd1, &c, 1);
		}
		write(test.fd1, ">\n", strlen(">\n"));
	}

	pid = fork();
	if (pid == 0)
	{
		test_children_init();
		if (test.fd0 >= 0)
		{
			close(0);
			dup2(test.fd0, 0);
		}
		if (test.fd1 >= 0)
		{
			close(1);
			dup2(test.fd1, 1);
		}
		if (test.fd2 >= 0)
		{
			close(2);
			dup2(test.fd1, 2);
		}
		close(test.fd[1]);

		ret = test.main[test_nb](argc + 1, argv - 1);
		if (ret == 0)
		{
			close(test.fd[0]);
			exit(0);
		}
		else
		{
			close(test.fd[0]);
			exit(1);
		}
	}
	close(test.fd[0]);
	error = " timeout";
	while (timeout.tv_sec > 0)
	{
		int status;
		fd_set set_other;
		fd_set set_read;
		FD_ZERO(&set_other);
		FD_ZERO(&set_read);
		FD_SET(test.fd[1], &set_other);
		FD_SET(test.fd[1], &set_read);
		ret = select(test.fd[1] + 1, &set_read, NULL, &set_other, &timeout);
		if (waitpid(pid, &status, WNOHANG) == pid)
		{
			ended = true;
			if (!WIFEXITED(status))
			{
				error= " unexpected exit";
				break;
			}
			if (WEXITSTATUS(status) != 0)
			{
				error= " test return error";
				break;
			}
			error = "";
			success = true;
			break;
		}
	}
	if (!ended)
	{
		int status;
		kill(pid, SIGKILL);
		waitpid(pid, &status, 0);
	}
	close(test.fd[1]);
	printf(" %10s (ran for %d s%s)\n", success?"SUCCESS":"FAILED", (int)(TEST_TIMEOUT - timeout.tv_sec), error);

	if (test.fd1 >= 0)
	{
		write(test.fd1, "--- Test result <", strlen("--- Test result <"));
		if (success)
			write(test.fd1, "SUCCESS", strlen("SUCCESS"));
		else
			write(test.fd1, "FAILED", strlen("FAILED"));
		write(test.fd1, ">\n", strlen(">\n"));
	}
	return success;
}

static bool test_run_all(int argc, char **argv)
{
	int i;
	int nb_success = 0;
	int nb_passed = 0;
	for (i = 0; i < test_max; i++)
	{
		bool result = test_run(test.name[i], argc, argv);
		if (result)
			nb_success++;
		nb_passed++;
	}
	printf("Test success %d/%d\n", nb_success, nb_passed);
	return (nb_success == nb_passed);
}

static void test_show_option(char *name)
{
	int i;
	printf("%s [output|output_log|output_log_error] [TESTNAME\n\trun TESTNAME test, TESTNAME can be\n", name);
	for (i = 0; i < test_max; i++)
		printf("\t%s\n", test.name[i]);
	printf("if neither output, out_log or output_log_error are specified, the command output is discarded\n");
	printf("if output_log is given, the command output are put in test.log\n");
	printf("if output_log_error is given, the command error output are put in test.log, stdout is discared\n");

	printf("if output is given, the command output is displayed\n");
	printf("if output_log is given, the command output are put in test.log\n");
	printf("if output_log_error is given, the command error output are put in test.log, stdout is discared\n");
}

static void test_register(int (*main)(int argc, char **argv), const char * name)
{
	const char * forbiden[] = {"output", "output_log", "output_log_error", "all", ""};
	int i = 0;
	if (test_max >= TEST_MAX)
		exit(1);
	while (strcmp(forbiden[i], "") != 0)
	{
		if (strcmp(forbiden[i], name) == 0)
		{
			printf("Internal error: A test use a forbidden name <%s>, exiting\n",
				name);
			exit(1);
		}
		i++;
	}
	if (strchr(name,' ') != NULL)
	{
		printf("Internal error: A test name contains space <%s>, exiting\n",
			name);
		exit(1);
	}
	test.main[test_max] = main;
	test.name[test_max] = name;
	test_max++;
}

int main_test_success(int argc, char **argv)
{
	return 0;
}

int main_test_failed(int argc, char **argv)
{
	return 1;
}

void test_init(void)
{
	int status;
	sigset_t sigset;
	struct sigaction sa;
	if (sigemptyset(&sigset) < 0)
		return;
	if (sigaddset(&sigset, SIGUSR1) < 0) 
		return;
	if (sigaddset(&sigset, SIGUSR2) < 0) 
		return;
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0)
		return;
}


int main(int argc, char **argv)
{
	test.fd0 = open("/dev/null", O_WRONLY);
	test.fd1 = open("/dev/null", O_RDONLY);
	test.fd2 = open("/dev/null", O_WRONLY);
	test_init();
	test_register(main_test_success ,"test_success");
	test_register(main_test_failed ,"test_failed");
	test_register(main_chew ,"chew");
	test_register(main_fiftyp ,"fiftyp");
	test_register(main_hackbench ,"hackbench");
	test_register(main_irman2 ,"irman2");
	test_register(main_massive_intr ,"massive_intr");
	test_register(main_onetest ,"onetest");
	test_register(main_starve ,"starve");
	test_register(main_tenp ,"tenp");
	test_register(main_testcase ,"testcase");
	test_register(main_thud2 ,"thud2");
	test_register(main_thud ,"thud");
	test_register(main_migrate ,"migrate_time");
	test_register(main_hmp_migration, "hmp_migration");
	test_register(main_affinity,"affinity_notchanged");
	test_register(main_cpuhog, "cpuhog");
	test_register(main_pipe_test, "pipe_test");
	test_register(main_pipe_test_1m, "pipe_test_1m");
	test_register(main_pth_str01, "main_pth_str01");
	test_register(main_pth_str02, "main_pth_str02");
	test_register(main_pth_str03, "main_pth_str03");
	if ((argc >= 3) && (strcmp(argv[1], "output") == 0))
	{
		close(test.fd1);
		close(test.fd2);
		test.fd1 = -1;
		test.fd2 = -1;
		argc--;
		argv++;
	}
	else if ((argc >= 3) && (strcmp(argv[1], "output_log") == 0))
	{
		close(test.fd1);
		close(test.fd2);
		test.fd1 = open("test.log", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
		test.fd2 = test.fd0;
		argc--;
		argv++;
	}
	else if ((argc >= 3) && (strcmp(argv[1], "output_log_error") == 0))
	{
		close(test.fd2);
		test.fd2 = open("test.log", O_CREAT | O_APPEND| O_WRONLY, S_IRWXU);
		argc--;
		argv++;
	}
  if (argc >= 2)
  {
	bool ret;
	int fd_android = 0;
	fd_android = open("/system/build.prop", O_RDONLY);
	if (fd_android != -1) {
		system("stop"); /* stop all android daemons and UI */
	}
	if (strcmp(argv[1], "all") == 0)
		ret = test_run_all(argc - 1, argv + 1);
	else
		ret = test_run(argv[1], argc - 1, argv + 1);
	if (fd_android != -1) {
		system("start"); /* restart all android daemons and UI */
		close(fd_android);
        }
	if (ret == true)
		return 0;
	return 1;
  }
  test_show_option(argv[0]); 
  return 1;
}
