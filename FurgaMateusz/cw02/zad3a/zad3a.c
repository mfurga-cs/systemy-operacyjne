/*
  Operating systems: lab2-3a
  Mateusz Furga <mfurga@student.agh.edu.pl>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define T_FILE 0
#define T_DIRECTORY 1
#define T_CHAR_SPEC_FILE 2
#define T_BLOCK_SPEC_FILE 3
#define T_FIFO_SPEC_FILE 4
#define T_SYMLINK 5
#define T_SOCKET 6

char path_buff[1024] = {0};
unsigned counts[7] = {0};

const char *file_type(struct stat *st)
{
  mode_t m = st->st_mode;

  if (S_ISREG(m)) {
    counts[T_FILE] += 1;
    return "FILE";
  }
  if (S_ISDIR(m)) {
    counts[T_DIRECTORY] += 1;
    return "DIRECTORY";
  }
  if (S_ISCHR(m)) {
    counts[T_CHAR_SPEC_FILE] += 1;
    return "CHARACTER SPECIAL FILE";
  }
  if (S_ISBLK(m)) {
    counts[T_BLOCK_SPEC_FILE] += 1;
    return "BLOCK SPECIAL FILE";
  }
  if (S_ISFIFO(m)) {
    counts[T_FILE] += 1;
    return "FIFO SPECIAL FILE";
  }
  if (S_ISLNK(m)) {
    counts[T_SYMLINK] += 1;
    return "SYMLINK";
  }
  if (S_ISSOCK(m)) {
    counts[T_SOCKET] += 1;
    return "SOCKET";
  }

  return "UNDEFINED";
}

void print_file(const char *fn)
{
  struct stat st;
  static char atime[20], mtime[20];

  lstat(fn, &st);

  strftime(atime, sizeof(atime), "%Y-%m-%d %H:%M:%S", localtime(&st.st_atime));
  strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));

  printf("%s\t%lu\t%s\t%lu\t%s\t%s\n", fn, st.st_nlink, file_type(&st),
                                       st.st_size, atime, mtime);
}

void scan(int offset)
{
  path_buff[offset] = '\0';

  DIR *d = opendir(path_buff);
  if (d == NULL) {
    fprintf(stderr, "Cannot open directory: %s\n", path_buff);
    return;
  }

  struct dirent *ent;

  while ((ent = readdir(d)) != NULL) {

    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }

    path_buff[offset] = '/';
    memcpy(path_buff + offset + 1, ent->d_name, strlen(ent->d_name));
    path_buff[offset + 1 + strlen(ent->d_name)] = '\0';

    print_file(path_buff);

    if (ent->d_type == DT_DIR) {
      scan(offset + 1 + strlen(ent->d_name));
    }

    path_buff[offset] = '\0';
  }

  closedir(d);
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "Usage %s <directory>\n", argv[0]);
    return 1;
  }

  realpath(argv[1], path_buff);
  int length = 0;
  while (path_buff[length] != 0) {
    length++;
  }

  scan(length);

  printf("\n    === STATS ===    \n"
         "Files:               %u\n"
         "Directories:         %u\n"
         "Char Special Files:  %u\n"
         "Block Special Files: %u\n"
         "Symlink:             %u\n"
         "Socket:              %u\n",
         counts[T_FILE], counts[T_DIRECTORY], counts[T_CHAR_SPEC_FILE],
         counts[T_BLOCK_SPEC_FILE], counts[T_SYMLINK], counts[T_SOCKET]);

  return 0;
}

