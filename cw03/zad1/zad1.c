#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <number of processes>\n", argv[0]);
    return 1;
  }

  int n = atoi(argv[1]);

  pid_t pids[n];

  for (int i = 0; i < n; i++) {
    pids[i] = fork();

    if (pids[i] == 0) {  /* Child. */
      printf("I am from %u\n", getpid());
      return 0;
    }
  }

  for (int i = 0; i < n; i++) {
    waitpid(pids[i], NULL, 0);
  }

  printf("All processes finished.\n");

  return 0;
}

