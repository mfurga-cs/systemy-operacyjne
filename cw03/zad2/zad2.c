#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

double f(double x)
{
  return 4.0 / (x * x + 1.0);
}

int worker(int i, int n)
{
  double area = (1.0 / (double)n) * f((double)i / (double)n);
  char fn[32] = {};
  char num[19];

  snprintf(fn, sizeof(fn), "w%d.txt", i);
  snprintf(num, sizeof(num), "%.16f", area);

  int fd = open(fn, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    fprintf(stderr, "Failed to open file to write %s\n", fn);
    return 1;
  }

  if (write(fd, num, sizeof(num)) != sizeof(num)) {
    fprintf(stderr, "Failed to write to file %s\n", fn);
    close(fd);
    return 1;
  }
  close(fd);

  return 0;
}

int main(int argc, char **argv)
{
  char fn[32] = {};
  char num[19];

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <number of processes>\n", argv[0]);
    return 1;
  }

  clock_t begin = clock();

  int n = atoi(argv[1]);
  pid_t pids[n];

  for (int i = 0; i < n; i++) {
    pids[i] = fork();

    if (pids[i] == 0) {
      /* Child. */
      return worker(i, n);
    }

    if (pids[i] == -1) {
      fprintf(stderr, "Failed to create a new process.\n");
      return 1;
    }
  }

  for (int i = 0; i < n; i++) {
    waitpid(pids[i], NULL, 0);
  }

  double sum = 0;

  for (int i = 0; i < n; i++) {
    snprintf(fn, sizeof(fn), "w%d.txt", i);

    int fd = open(fn, O_RDONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      fprintf(stderr, "Failed to open file to read %s\n", fn);
      return 1;
    }

    if (read(fd, num, sizeof(num)) != sizeof(num)) {
      fprintf(stderr, "Failed to read from %s\n", fn);
      close(fd);
      return 1;
    }

    close(fd);

    sum += strtod(num, NULL);
  }

  clock_t end = clock();
  printf("%.16f\n", sum);
  printf("Time: %f\n",  (double)(end - begin) / CLOCKS_PER_SEC);

  return 0;
}

