#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static volatile unsigned long loop = 1000000;
static int father_pid;

static void
handler (int n)
{
  if (loop > 0)
    --loop;
}

static int
child (void)
{
  sleep (1);
  while (1)
  {
    kill (father_pid, SIGUSR1);
  }
  return 0;
}

int main_starve(int argc, char **argv)
{
  pid_t child_pid;
  int r;
  struct sigaction sa;
  father_pid = getpid();

  printf ("expecting to receive %lu signals\n", loop);

  if ((child_pid = fork ()) == 0)
    exit (child ());

  sa.sa_handler = handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction (SIGUSR1, &sa, NULL);


  sigaddset(&sa.sa_mask, SIGUSR1);
  sigprocmask(SIG_UNBLOCK, &sa.sa_mask, NULL);

  while (loop)
    sleep (1);
  r = kill (child_pid, SIGTERM);
  waitpid (child_pid, NULL, 0);
  return 0;
}
