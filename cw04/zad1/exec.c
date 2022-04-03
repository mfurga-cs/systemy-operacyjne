#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int param_ignore(void)
{
  printf("exec: running ignore\n");
  raise(SIGUSR1);
  return 0;
}

int param_mask(void)
{
  sigset_t childmask;
  sigemptyset(&childmask);
  if (sigprocmask(SIG_SETMASK, NULL, &childmask) == -1) {
    printf("sigprocmask failed.\n");
    return 1;
  }

  if (sigdelset(&childmask, SIGUSR1) == -1) {
    printf("sigdelset failed.\n");
    return 1;
  }

  if (sigisemptyset(&childmask) == 1) {
    printf("exec: child has the same set as parent.\n");
  } else {
    printf("exec: child does not have the same set as parent.\n");
  }

  return 0;
}

int param_pending(void)
{
  sigset_t mask;
  sigemptyset(&mask);
  if (sigpending(&mask) == -1) {
    printf("sigpending failed.\n");
    return 1;
  }

  int res = sigismember(&mask, SIGUSR1);
  if (res == -1) {
    printf("sigismember failed.\n");
    return 1;
  } else if (res == 1) {
    printf("exec: SIGUSR1 is pending in child.\n");
  } else {
    printf("exec: SIGUSR1 is not pending in child.\n");
  }

  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: %s <ignore|handler|mask|pending>\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "ignore") == 0) {
    return param_ignore();
  } else if (strcmp(argv[1], "mask") == 0) {
    return param_mask();
  } else if (strcmp(argv[1], "pending") == 0) {
    return param_pending();
  } else {
    printf("Invalid parameter.\n");
    return 1;
  }

  return 0;
}


