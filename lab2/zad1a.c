/*
  Operating systems: lab2-1a
  Mateusz Furga <mfurga@student.agh.edu.pl>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Debug and error macros */

#ifndef MESSAGES_TO_STDERR
#  define SAYF(x...) printf(x)
#else
#  define SAYF(x...) fprintf(stderr, x)
#endif

#define DEBUGF(x...) do { \
    SAYF("[*] " x); \
    SAYF("\n"); \
  } while (0)

#define FATALF(x...) do { \
    SAYF("[-] " x); \
    SAYF("\n"); \
  } while (0)

int remove_empty_lines(const char *in, const char *out)
{
  int buff_sz;
  char *buff_in;
  char *buff_out;

  FILE *fin = fopen(in, "r");
  if (fin == NULL) {
    FATALF("Cannot open %s to read", in);
    return 1;
  }

  FILE *fout = fopen(out, "w");
  if (fout == NULL) {
    FATALF("Cannot open %s to write", out);

    /* NOTE: OS will release all open descriptors, allocated memory and
             other stuff after killing the process. Therefore we can skip
             freeing memory and closing files. */
    fclose(fin);

    return 1;
  }

  fseek(fin, 0, SEEK_END);
  buff_sz = ftell(fin);
  fseek(fin, 0, SEEK_SET);

  buff_in = malloc(buff_sz);
  if (buff_in == NULL) {
    FATALF("Run out of memory");

    /* Note above. */
    fclose(fin);
    fclose(fout);

    return 1;
  }

  buff_out = malloc(buff_sz);
  if (buff_out == NULL) {
    FATALF("Run out of memory");

    /* Note above. */
    fclose(fin);
    fclose(fout);
    free(buff_in);

    return 1;
  }

  if (fread(buff_in, 1, buff_sz, fin) != buff_sz) {
    FATALF("Unable to read %s file", in);

    /* Note above. */
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

  if (argc < 3) {
    printf("IN: ");
    scanf("%255s", in);
    printf("OUT: ");
    scanf("%255s", out);
  } else if (argc == 3) {
    in_sz = strlen(argv[1]);
    out_sz = strlen(argv[2]);

    if (in_sz >= 256 || out_sz >= 256) {
      FATALF("Given filename is too long.");
      return 1;
    }

    memcpy(in, argv[1], in_sz);
    memcpy(out, argv[2], out_sz);
  } else {
    FATALF("Usage %s <in> <out>", argv[0]);
    return 1;
  }

  return remove_empty_lines(in, out);
}

