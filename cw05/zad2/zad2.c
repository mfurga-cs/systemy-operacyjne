#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_mail(char *sort) {
  FILE *handler;

  if (strcmp(sort, "nadawca") == 0) {
    handler = popen("mail | sort -k3", "w");
  } else if (strcmp(sort, "data") == 0) {
    /* Already sorted. */
    handler = popen("mail", "w");
  } else {
    return 1;
  }

  if (handler == NULL) {
    fprintf(stderr, "popen failed.\n");
    return 1;
  }

  fputs("exit", handler);
  pclose(handler);
  return 0;
}

int write_mail(char *email, char *subject, char *message) {
  char command[64];
  snprintf(command, sizeof(command), "mail -s '%s' %s", subject, email);

  FILE *handler = popen(command, "w");
  if (handler == NULL) {
    fprintf(stderr, "popen failed.\n");
    return 1;
  }

  fputs(message, handler);
  pclose(handler);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    return read_mail(argv[1]);
  } else if (argc == 4) {
    return write_mail(argv[1], argv[2], argv[3]);
  } else {
    printf("Usage: %s (<nadawca|data>) | (<email> <title> <message>)\n", argv[0]);
    return 1;
  }
  return 0;
}

