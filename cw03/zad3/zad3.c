#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>

char abs_path[16 * 1024] = {0};
int start_idx = 0;

int search(int offset, char *pattern, int depth)
{
  struct dirent *ent;

  DIR *d = opendir(abs_path);
  if (d == NULL) {
    fprintf(stderr, "Cannot open directory: %s\n", abs_path);
    return 1;
  }

  while ((ent = readdir(d)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }

    if (offset + 1 + strlen(ent->d_name) >= sizeof(abs_path)) {
      fprintf(stderr, "Buffor too small\n");
      closedir(d);
      return 1;
    }

    abs_path[offset] = '/';
    memcpy(abs_path + offset + 1, ent->d_name, strlen(ent->d_name));
    abs_path[offset + 1 + strlen(ent->d_name)] = '\0';

    printf("%s\n", abs_path);

    if (ent->d_type == DT_DIR) {
      pid_t pid = fork();
      if (pid == 0) {  /* Child. */
        printf("Child process created for %s\n", abs_path + start_idx);
        return search(offset + 1 + strlen(ent->d_name), pattern, depth - 1);
      }
    }
  }

  closedir(d);
  return 0;
}

int main(int argc, char **argv)
{
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <directory> <pattern> <max depth>\n", argv[0]);
    return 1;
  }

  char *path = realpath(argv[1], NULL);
  if (path == NULL) {
    fprintf(stderr, "Realpath error\n");
    return 1;
  }

  if (strlen(path) >= sizeof(abs_path)) {
    fprintf(stderr, "Buffor too small\n");
    return 1;
  }

  memcpy(abs_path, path, strlen(path));
  start_idx = strlen(path) + 1;
  free(path);

  int depth = atoi(argv[3]);

  pid_t pid = fork();

  if (pid == 0) {
    return search(strlen(path), path, depth);
  }

  printf("Process created (pid=%u)\n", pid);

  return 0;
}

