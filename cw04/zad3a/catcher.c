#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

int SIGNAL1 = SIGUSR1;
int SIGNAL2 = SIGUSR2;

int recv = 0;
int end = 0;
int pid = 0;

void send_kill(void)
{
  for (int i = 0; i < recv; i++) {
    kill(pid, SIGNAL1);
  }
  kill(pid, SIGNAL2);
}

void send_sigqueue(void)
{
  union sigval value;
  for (int i = 0; i < recv; i++) {
    value.sival_int = i;
    sigqueue(pid, SIGNAL1, value);
  }
  value.sival_int = recv;
  sigqueue(pid, SIGNAL2, value);
}

void send_sigrt(void)
{
  for (int i = 0; i < recv; i++) {
    kill(pid, SIGNAL1);
  }
  kill(pid, SIGNAL2);
}

void handle_signal1(int sig)
{
  (void)sig;
  recv++;
}

void handle_signal2(int sig, siginfo_t *info, void *ucontext)
{
  (void)ucontext;
  (void)sig;
  pid = info->si_pid;
  end = 1;
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: %s <KILL|SIGQUEUE|SIGRT>\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "SIGRT") == 0) {
    SIGNAL1 = SIGRTMIN;
    SIGNAL2 = SIGRTMIN + 1;
  }

  sigset_t mask;
  sigemptyset(&mask);
  sigfillset(&mask);
  sigdelset(&mask, SIGNAL1);
  sigdelset(&mask, SIGNAL2);

  printf("PID: %d\n", getpid());

  if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1) {
    printf("sigprocmask failed.\n");
    return 1;
  }

  signal(SIGNAL1, handle_signal1);

  struct sigaction act;
  act.sa_sigaction = handle_signal2;
  act.sa_mask = mask;
  act.sa_flags = SA_SIGINFO;

  sigaction(SIGNAL2, &act, NULL);

  /* Wait for signals */
  while (!end) {
    sigsuspend(&mask);
  }

  if (strcmp(argv[1], "KILL") == 0) {
    send_kill();
  } else if (strcmp(argv[1], "SIGQUEUE") == 0) {
    send_sigqueue();
  } else if (strcmp(argv[1], "SIGRT") == 0) {
    send_sigrt();
  }

  printf("Received signals: %d\n", recv);

  return 0;
}

