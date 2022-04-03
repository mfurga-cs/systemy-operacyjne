#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void handler_SIGTSTP(int sign)
{
  printf("Received signal: SIGTSTP(%d)\n", sign);
  printf("Signal will no longer be handled\n");
}

void handler_SIGCHLD(int sign)
{
  printf("Received signal: SIGCHLD(%d)\n", sign);
}

void handler_SIGINT(int sig, siginfo_t *info, void *ucontext)
{
  (void)ucontext;
  printf("Received signal: SIGINT(%d) from %d\n", sig, info->si_pid);
}

int main(void)
{
  printf("Current PID: %d\n", getpid());

  struct sigaction act;

  /* Handler for SIGTSTP */
  act.sa_handler = handler_SIGTSTP;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESETHAND;
  sigaction(SIGTSTP, &act, NULL);

  /* Handler for SIGCHLD */
  act.sa_handler = handler_SIGCHLD;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_NOCLDSTOP;
  sigaction(SIGCHLD, &act, NULL);

  /* Handler for SIGINT */
  act.sa_sigaction = handler_SIGINT;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &act, NULL);

  /* Waiting */

  puts("Waiting for signals ...");
  puts("Creating a new child process to test SIGCHLD signal ...");

  pid_t pid = fork();
  if (pid == 0) {
    printf("Child process created: %d\n", getpid());
    close(1);
  }

	for (;;);
	return 0;
}

