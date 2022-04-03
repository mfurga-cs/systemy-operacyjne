#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void handler(int sig)
{
  printf("Received signal: SIGUSR1(%d), PID(%d)\n", sig, getpid());
}

int param_ignore(void)
{
  signal(SIGUSR1, SIG_IGN);

  raise(SIGUSR1);
  pid_t pid = fork();
  if (pid == -1) {
    printf("Failed to create child process.\n");
    return 1;
  }

  if (pid == 0) {
    raise(SIGUSR1);

    execl("exec", "exec", "ignore", NULL);
    return 1;
  }

  return 0;
}

int param_handler(void)
{
  signal(SIGUSR1, handler);
  raise(SIGUSR1);

  pid_t pid = fork();
  if (pid == -1) {
    printf("Failed to create child process.\n");
    return 1;
  }

  if (pid == 0) {
    raise(SIGUSR1);
  }

  return 0;
}

int param_mask(void)
{
  sigset_t newmask;
  sigemptyset(&newmask);
  sigaddset(&newmask, SIGUSR1);

  if (sigprocmask(SIG_SETMASK, &newmask, NULL) == -1) {
    printf("sigprocmask failed.\n");
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    printf("Failed to create child process.\n");
    return 1;
  }

  if (pid == 0) {
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
      printf("fork: child has the same set as parent.\n");
    } else {
      printf("fork: child does not have the same set as parent.\n");
    }

    sigaddset(&childmask, SIGUSR1);
    execl("exec", "exec", "mask", NULL);
  }

  return 0;
}

int param_pending(void)
{
  sigset_t newmask;
  int res;
  sigemptyset(&newmask);
  sigaddset(&newmask, SIGUSR1);

  if (sigprocmask(SIG_SETMASK, &newmask, NULL) == -1) {
    printf("sigprocmask failed.\n");
    return 1;
  }

  raise(SIGUSR1);

  sigset_t penmask;
  sigemptyset(&penmask);

  if (sigpending(&penmask) == -1) {
    printf("sigpending failed.\n");
    return 1;
  }

  res = sigismember(&penmask, SIGUSR1);
  if (res == -1) {
    printf("sigismember failed.\n");
    return 1;
  } else if (res == 1) {
    printf("SIGUSR1 is pending in parent.\n");
  } else {
    printf("SIGUSR1 is not pending in parent.\n");
  }

  pid_t pid = fork();
  if (pid == -1) {
    printf("Failed to create child process.\n");
    return 1;
  }

  if (pid == 0) {
    sigemptyset(&penmask);
    if (sigpending(&penmask) == -1) {
      printf("sigpending failed.\n");
      return 1;
    }

    res = sigismember(&penmask, SIGUSR1);
    if (res == -1) {
      printf("sigismember failed.\n");
      return 1;
    } else if (res == 1) {
      printf("fork: SIGUSR1 is pending in child.\n");
    } else {
      printf("fork: SIGUSR1 is not pending in child.\n");
    }

    execl("exec", "exec", "pending", NULL);
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
  } else if (strcmp(argv[1], "handler") == 0) {
    return param_handler();
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


