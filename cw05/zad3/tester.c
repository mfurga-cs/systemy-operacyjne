#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <number of writers> <number of readers> <number of chars>\n", argv[0]);
    return 1;
  }

  int writers = atoi(argv[1]);
  int readers = atoi(argv[2]);
  int chars = atoi(argv[3]);

  if (writers > 9) {
    fprintf(stderr, "to many writers.\n");
    return 1;
  }

  pid_t pids[writers + readers];
  int i;

  printf("FIFO BUFFER SIZE: %d\n", PIPE_BUF);

  mkfifo("fifo", 0666);

  for (i = 0; i < writers; i++) {
    pids[i] = fork();
    if (pids[i] == 0) {
      char p_id[2];
      char p_file[32];
      char p_chars[32];
      snprintf(p_id, sizeof(p_id), "%d", i);
      snprintf(p_file, sizeof(p_file), "data/%d.txt", i);
      snprintf(p_chars, sizeof(p_chars), "%d", chars);

      printf("WRITER: %s\n", p_id);
      execl("writer", "writer", "fifo", p_id, p_file, p_chars, NULL);

      printf("worker %d failed.\n", i);
      _exit(1);
    }
  }

  sleep(1);

  for (; i < writers + readers; i++) {
    pids[i] = fork();
    if (pids[i] == 0) {
      char p_chars[32];
      snprintf(p_chars, sizeof(p_chars), "%d", chars);

      printf("READER\n");
      execl("reader", "reader", "fifo", "data/res.txt", p_chars, NULL);
      printf("reader %d failed.\n", i);
      _exit(1);
    }
  }

  int status;
  for (i = 0; i < writers + readers; i++) {
    waitpid(pids[i], &status, NULL);
  }

  return 0;
}

