#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define READ_BUFF_SIZE 1024 * 16
#define WRITE_BUFF_SIZE 1024 * 16

char read_buff[READ_BUFF_SIZE];
char write_buff[WRITE_BUFF_SIZE];

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s <pipe fifo> <row number 0..9> <filename> <number of chars>\n", argv[0]);
    return 1;
  }

  /* row can be only a digit 0..9 */
  int row = atoi(argv[2]), num_chars = atoi(argv[4]);
  size_t read, write;

  FILE *file_handler = fopen(argv[3], "r");
  if (file_handler == NULL) {
    fprintf(stderr, "fopen failed.\n");
    return 1;
  }

  setvbuf(file_handler, NULL, _IOLBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
 
  while ((read = fread(read_buff, 1, num_chars, file_handler)) > 0) {
    read_buff[read] = '\0';
    write = snprintf(write_buff, sizeof(write_buff), "%d%s", row, read_buff);
    printf("WRITER(%d): %s\n", row, write_buff);

    while (write != num_chars + 1) {
      write_buff[write++] = '\0';
    }

    printf("writen: %d\n", write);

    FILE *pipe_handler = fopen(argv[1], "w");
    if (pipe_handler == NULL) {
      fprintf(stderr, "fopen failed.\n");
      return 1;
    }
    fwrite(write_buff, 1, num_chars + 1, pipe_handler);
    //sleep(1 + (rand() + RAND_MAX - 1) / RAND_MAX);
    fclose(pipe_handler);
  }

  printf("WRITER CLOSE %d\n", row);

  fclose(file_handler);
  return 0;
}

