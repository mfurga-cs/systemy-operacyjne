#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv, char **envp)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <number of processes>\n", argv[0]);
    return 1;
  }

  int n = atoi(argv[1]);

  pid_t pids[n];

  for (int i = 0; i < n; i++) {
    pids[i] = fork();

    if (pids[i] == 0) {
      /* Child. */

      char buff[128];
      snprintf(buff, sizeof(buff), "I am from %i", i);

      char *nargv[] = {
        "/bin/echo",
        buff,
        NULL
      };

      /* This function never returns. */
      execve(nargv[0], nargv, envp);

      fprintf(stderr, "Failed to create process.\n");
      return 1;
    }
  }

  for (int i = 0; i < n; i++) {
    waitpid(pids[i], NULL, 0);
  }

  printf("All processes finished.\n");

  return 0;
}

