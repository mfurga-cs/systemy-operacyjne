#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

double f(double x) {
  return 4.0 * / (x * x + 1.0);
}

int worker(int w, int i) {
  printf("Hello world %d %d\n", w, i);

  return 1;
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <width> <number of processes>\n", argv[0]);
    return 1;
  }

  int w = atoi(argv[1]);
  int n = atoi(argv[2]);

  pid_t pids[n];

  for (int i = 0; i < n; i++) {
    pids[i] = fork();

    if (pids[i] == 0) {
      /* Child. */
      return worker(w, i);
    }
  }

  for (int i = 0; i < n; i++) {
    waitpid(pids[i], NULL, 0);
  }

  printf("All processes finished.\n");

  return 0;
}

