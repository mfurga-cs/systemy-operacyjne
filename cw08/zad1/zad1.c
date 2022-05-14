#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

int w, h;
unsigned char *bitmap_in;
unsigned char *bitmap_out;
ssize_t sz;

typedef struct {
  int from;
  int to;
} interval_t;

void *invert_numbers_thread(void *interval) {
  clock_t begin = clock();

  int from = ((interval_t *)interval)->from;
  int to = ((interval_t *)interval)->to;

  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      if (bitmap_in[i * h + j] >= from && bitmap_in[i * h + j] <= to) {
        bitmap_out[i * h + j] = 255 - bitmap_in[i * h + j];
      }
    }
  }

  double *time_spent = malloc(sizeof(double));
  clock_t end = clock();
  *time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

  pthread_exit(time_spent);
}

void *invert_block_thread(void *interval) {
  clock_t begin = clock();

  int from = ((interval_t *)interval)->from;
  int to = ((interval_t *)interval)->to;

  for (int i = from; i <= to; i++) {
    bitmap_out[i] = 255 - bitmap_in[i];
  }

  double *time_spent = malloc(sizeof(double));
  clock_t end = clock();
  *time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

  pthread_exit(time_spent);
}

void invert_numbers(int n) {
  interval_t intervals[n];
  int len = 256 / n;

  int next = 0;
  for (int i = 0; i < n; i++) {
    intervals[i].from = next;
    intervals[i].to = next + len - 1;
    if (i == n - 1) {
      intervals[i].to = 255;
    }
    next += len;
  }

  pthread_t threads[n];

  for (int i = 0; i < n; i++) {
    if (pthread_create(&threads[i], NULL, invert_numbers_thread,
                       &intervals[i]) != 0) {
      puts("Failed to create a new thread.");
      return;
    }
  }

  void *ret;
  for (int i = 0; i < n; i++) {
    pthread_join(threads[i], &ret);
    printf("Thread %d: %.16fs\n", i, *((double *)ret));
    free(ret);
  }
}

void invert_block(int n) {
  int len = w * h / n;
  interval_t intervals[n];

  int next = 0;
  for (int i = 0; i < n; i++) {
    intervals[i].from = next;
    intervals[i].to = next + len - 1;
    if (i == n - 1) {
      intervals[i].to = w * h - 1;
    }
    next += len;
  }

  pthread_t threads[n];

  for (int i = 0; i < n; i++) {
    if (pthread_create(&threads[i], NULL, invert_block_thread,
                       &intervals[i]) != 0) {
      puts("Failed to create a new thread.");
      return;
    }
  }

  void *ret;
  for (int i = 0; i < n; i++) {
    pthread_join(threads[i], &ret);
    printf("Thread %d: %.16fs\n", i, *((double *)ret));
    free(ret);
  }
}

int read_pgm(const char *fname) {

  int fd = open(fname, O_RDONLY);
  if (fd == -1) {
    puts("Failed to open the input file.");
    return 1;
  }

  sz = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  char *buff = malloc(sz);
  if (buff == NULL) {
    puts("Out of memory.");
    close(fd);
    return 1;
  }

  if (read(fd, buff, sz) != sz) {
    puts("Failed to read the input file.");
    close(fd);
    return 1;
  }

  int idx = 0;

  if (buff[idx] == 'P' && buff[idx + 1] == '2' && buff[idx + 2] == '\n') {
    idx += 3;
  }

  w = 0;
  while (buff[idx] != ' ') {
    w = w * 10 + (buff[idx] - '0');
    idx++;
  }
  idx += 1;

  h = 0;
  while (buff[idx] != '\n') {
    h = h * 10 + (buff[idx] - '0');
    idx++;
  }
  idx += 1;

  while (buff[idx] != '\n') {
    idx++;
  }
  idx++;

  bitmap_in = malloc(w * h);
  bitmap_out = malloc(w * h);

  if (bitmap_in == NULL || bitmap_out == NULL) {
    puts("Failed to read the input file.");
    close(fd);
    return 1;
  }

  unsigned char v;
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      v = 0;
      while (idx < sz && buff[idx] >= '0' && buff[idx] <= '9') {
        v = v * 10 + (buff[idx] - '0');
        idx++;
      }
      idx++;
      bitmap_in[i * h + j] = v;
    }
  }

  free(buff);
  close(fd);
  return 0;
}

int write_pgm(const char *fname) {

  int fd = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    puts("Failed to open the output file.");
    return 1;
  }

  ssize_t size = 3 * sz;
  char *buff = malloc(size);
  if (buff == NULL) {
    puts("Out of memory.");
    return 1;
  }

  int idx = 0;
  buff[idx++] = 'P';
  buff[idx++] = '2';
  buff[idx++] = '\n';

  idx += snprintf(buff + idx, size - idx, "%d %d\n", w, h);
  buff[idx++] = '2';
  buff[idx++] = '5';
  buff[idx++] = '5';
  buff[idx++] = '\n';

  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      if (j == w - 1) {
        idx += snprintf(buff + idx, size - idx, "%d\n", bitmap_out[i * h + j]);
      } else {
        idx += snprintf(buff + idx, size - idx, "%d ", bitmap_out[i * h + j]);
      }
    }
  }

  if (write(fd, buff, idx) != idx) {
    puts("Failed to wrtie.\n");
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}

int main(int argc, char *argv[]) {

  if (argc != 5) {
    printf("Usage: %s <n> <numbers|block> <in> <out>\n", argv[0]);
    return 1;
  }

  clock_t begin = clock();

  int n = atoi(argv[1]);

  const char *in = argv[3];
  const char *out = argv[4];

  if (read_pgm(in) != 0) {
    return 1;
  }

  if (strcmp(argv[2], "numbers") == 0) {
    invert_numbers(n);
  } else if (strcmp(argv[2], "block") == 0) {
    invert_block(n);
  } else {
    puts("Wrong parameter.");
    return 1;
  }

  if (write_pgm(out) != 0) {
    return 1;
  }

  free(bitmap_in);
  free(bitmap_out);

  clock_t end = clock();
  double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

  printf("Total time: %.16fs\n", time_spent);
  return 0;
}

