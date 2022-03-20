#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int remove_empty_lines(const char *in, const char *out)
{
  unsigned buff_sz;
  char *buff_in;
  char *buff_out;

  FILE *fin = fopen(in, "r");
  if (fin == NULL) {
    fprintf(stderr, "Cannot open %s to read", in);
    return 1;
  }

  FILE *fout = fopen(out, "w");
  if (fout == NULL) {
    fprintf(stderr, "Cannot open %s to write", out);

    fclose(fin);
    return 1;
  }

  fseek(fin, 0, SEEK_END);
  buff_sz = ftell(fin);
  fseek(fin, 0, SEEK_SET);

  buff_in = malloc(buff_sz);
  if (buff_in == NULL) {
    fprintf(stderr, "Run out of memory");

    fclose(fin);
    fclose(fout);

    return 1;
  }

  buff_out = malloc(buff_sz);
  if (buff_out == NULL) {
    fprintf(stderr, "Run out of memory");

    fclose(fin);
    fclose(fout);
    free(buff_in);

    return 1;
  }

  if (fread(buff_in, 1, buff_sz, fin) != buff_sz) {
    fprintf(stderr, "Unable to read %s file", in);

    fclose(fin);
    fclose(fout);
    free(buff_in);
    free(buff_out);

    return 1;
  }

  fclose(fin);

  /* Assume that endlines are represented as the 0x0a (LF) byte. */

  int whitespace_char_line = 1;
  int line_begin = 0;
  int write_ptr = 0;

  for (unsigned i = 0; i < buff_sz; i++) {
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

  fwrite(buff_out, 1, write_ptr, fout);
  fclose(fout);

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
      fprintf(stderr, "Given filename is too long.");
      return 1;
    }

    memcpy(in, argv[1], in_sz);
    memcpy(out, argv[2], out_sz);
  }

  return remove_empty_lines(in, out);
}

