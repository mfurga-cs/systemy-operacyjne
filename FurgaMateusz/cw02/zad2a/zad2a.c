#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "Usage %s <character> <filename>\n", argv[0]);
    return 1;
  }

  if (strlen(argv[1]) != 1) {
    fprintf(stderr, "Second argument must be a character\n");
    return 1;
  }

  char c = argv[1][0];
  char *fn = argv[2];

  FILE *f = fopen(fn, "r");
  if (f == NULL) {
    fprintf(stderr, "Cannot open %s to read\n", fn);
    return 1;
  }

  int read;
  char buff[1024];

  unsigned char_ctr = 0;
  unsigned line_ctr = 0;
  int first_in_line = 1;

  while ((read = fread(buff, 1, sizeof(buff), f)) > 0) {
    for (int i = 0; i < read; i++) {
      if (buff[i] == c) {
        char_ctr++;
        if (first_in_line == 1) {
          line_ctr++;
          first_in_line = 0;
        }
      }
      if (buff[i] == '\n') {
        first_in_line = 1;
      }
    }
  }

  fclose(f);

  printf("Characters count: %u\n"
         "Lines count:    : %u\n", char_ctr, line_ctr);

  return 0;
}

