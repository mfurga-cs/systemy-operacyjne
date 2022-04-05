#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

int SIGNAL1 = SIGUSR1;
int SIGNAL2 = SIGUSR2;

int recv = 0;
int pid = 0;
int end = 0;

void handle_ack(int sig, siginfo_t *info, void *ucontext)
{
  (void)sig;
  (void)info;
  (void)ucontext;
  return;
}

void handle(int sig, siginfo_t *info, void *ucontext)
{
  (void)ucontext;
  printf("Received from PID(%d) SIGVALUE(%d)\n", info->si_pid, info->si_value.sival_int);

  if (sig == SIGNAL2) {
    pid = info->si_pid;
    end = 1;
  } else if (sig == SIGNAL1) {
    recv++;
  }
}

void send_kill(int pid, int num)
{
  sigset_t mask;
  sigfillset(&mask);
  sigdelset(&mask, SIGNAL1);

  for (int i = 0; i < num; i++) {
    kill(pid, SIGNAL1);
    sigsuspend(&mask);
  }
  kill(pid, SIGNAL2);
}

void send_sigqueue(int pid, int num)
{
  sigset_t mask;
  sigfillset(&mask);
  sigdelset(&mask, SIGNAL1);

  union sigval value;
  for (int i = 0; i < num; i++) {
    value.sival_int = i;
    sigqueue(pid, SIGNAL1, value);
    sigsuspend(&mask);
  }
  value.sival_int = num;
  sigqueue(pid, SIGNAL2, value);
}

void send_sigrt(int pid, int num)
{
  for (int i = 0; i < num; i++) {
    kill(pid, SIGNAL1);
  }
  kill(pid, SIGNAL2);
}

int main(int argc, char *argv[])
{
  if (argc != 4) {
    printf("Usage: %s <catcher PID> <number of signals> <mode>\n", argv[0]);
    return 1;
  }

  //printf("%d\n", getpid());

  int pid = atoi(argv[1]);
  int num = atoi(argv[2]);

  sigset_t mask;
  sigemptyset(&mask);

  sigaddset(&mask, SIGNAL1);
  sigaddset(&mask, SIGNAL2);

  struct sigaction act;
  act.sa_sigaction = handle_ack;
  act.sa_mask = mask;
  act.sa_flags = SA_SIGINFO;

  sigaction(SIGNAL1, &act, NULL);

  if (strcmp(argv[3], "KILL") == 0) {
    send_kill(pid, num);
  } else if (strcmp(argv[3], "SIGQUEUE") == 0) {
    send_sigqueue(pid, num);
  } else if (strcmp(argv[3], "SIGRT") == 0) {
    SIGNAL1 = SIGRTMIN;
    SIGNAL2 = SIGRTMIN + 1;
    send_sigrt(pid, num);
  }

  act.sa_sigaction = handle;
  sigaction(SIGNAL1, &act, NULL);
  sigaction(SIGNAL2, &act, NULL);

  /* Wait for signals */
  sigemptyset(&mask);
  while (!end) {
    sigsuspend(&mask);
  }

  printf("Expected number of signals: %d, received %d\n", num, recv);

  return 0;
}
