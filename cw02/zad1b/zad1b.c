#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int remove_empty_lines(const char *in, const char *out)
{
  int buff_sz;
  char *buff_in;
  char *buff_out;

  int din = open(in, O_RDONLY);
  if (din == -1) {
    fprintf(stderr, "Cannot open %s to read\n", in);
    return 1;
  }

  int dout = open(out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (dout == -1) {
    fprintf(stderr, "Cannot open %s to write\n", out);

    close(din);
    return 1;
  }

  lseek(din, 0, SEEK_END);
  buff_sz = lseek(din, 0, SEEK_CUR);
  lseek(din, 0, SEEK_SET);

  buff_in = malloc(buff_sz);
  if (buff_in == NULL) {
    fprintf(stderr, "Run out of memory\n");

    close(din);
    close(dout);
    return 1;
  }

  buff_out = malloc(buff_sz);
  if (buff_out == NULL) {
    fprintf(stderr, "Run out of memory\n");

    close(din);
    close(dout);
    free(buff_in);
    return 1;
  }

  if (read(din, buff_in, buff_sz) != buff_sz) {
    fprintf(stderr, "Unable to read %s file\n", in);

    close(din);
    close(dout);
    free(buff_in);
    free(buff_out);
    return 1;
  }

  close(din);

  /* Assume that endlines are represented as the 0x0a (LF) byte. */

  int whitespace_char_line = 1;
  int line_begin = 0;
  int write_ptr = 0;

  for (int i = 0; i < buff_sz; i++) {
    if (buff_in[i] == '\n') {

      if (whitespace_char_line == 0) {
        memcpy(buff_out + write_ptr, buff_in + line_begin, (i - line_begin + 1));
        write_ptr += (i - line_begin + 1);
      }

      whitespace_char_line = 1;
      line_begin = i + 1;
      continue;
    }

    if (!isspace(buff_in[i])) {
      whitespace_char_line = 0;
    }
  }

  if (write(dout, buff_out, write_ptr) != write_ptr) {
    fprintf(stderr, "Unable to write %s file\n", out);

    close(dout);
    free(buff_in);
    free(buff_out);
    return 1;
  }


  close(dout);

  free(buff_in);
  free(buff_out);

  return 0;
}

int main(int argc, char *argv[])
{
  char in[256], out[256];
  int in_sz, out_sz;

  if (argc != 3) {
    printf("IN: ");
    (void)!scanf("%255s", in);
    printf("OUT: ");
    (void)!scanf("%255s", out);
  } else {
    in_sz = strlen(argv[1]);
    out_sz = strlen(argv[2]);

    if (in_sz >= 256 || out_sz >= 256) {
      fprintf(stderr, "Given filename is too long\n");
      return 1;
    }

    memcpy(in, argv[1], in_sz);
    memcpy(out, argv[2], out_sz);
  }

  return remove_empty_lines(in, out);
}

