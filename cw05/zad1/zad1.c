#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define COMPONENT_MAX_LENGTH 32
#define PROG_MAX_LENGTH 32
#define ARGV_MAX_LENGTH 32
#define ARG_MAX_LENGTH 32

#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct command_s {
  char prog[PROG_MAX_LENGTH];
  char *argv[ARGV_MAX_LENGTH];
  struct command_s *next;
} command_t;

typedef struct component_s {
  struct command_s *head;
} component_t;

command_t *command_new(void) {
  command_t *c = calloc(1, sizeof(command_t));
  for (int i = 0; i < ARGV_MAX_LENGTH; i++) {
    c->argv[i] = calloc(ARG_MAX_LENGTH, sizeof(char));
  }
  c->next = NULL;
  return c;
}

void component_append(component_t *comp, command_t *new) {
  if (comp->head == NULL) {
    comp->head = new;
    return;
  }
  command_t *c = comp->head;
  while (c->next != NULL) {
    c = c->next;
  }
  c->next = new;
}

void component_merge(component_t *comp1, component_t *comp2) {
  if (comp1->head == NULL) {
    comp1->head = comp2->head;
    return;
  }
  command_t *b = comp1->head;
  while (b->next != NULL) {
    b = b->next;
  }
  b->next = comp2->head;
}

command_t *parse_command_line(char *command, int length) {
  command_t *c = command_new();
  int start_idx = 0;
  int parse_program_name = 1;
  int arg = 0;

  for (int i = 0; i < length; i++) {
    if (parse_program_name == 1 && command[i] == ' ') {
      memcpy(c->prog, command, i);
      memcpy(c->argv[arg], command, i);

      parse_program_name = 0;
      start_idx = i + 1;
      arg++;
      continue;
    }

    if (command[i] == ' ') {
      memcpy(c->argv[arg], command + start_idx, i - start_idx);
      start_idx = i + 1;
      arg++;
    }
  }

  if (parse_program_name == 1) {
    memcpy(c->prog, command, length);
  }

  memcpy(c->argv[arg], command + start_idx, length - start_idx);
  arg++;
  c->argv[arg] = NULL;

  return c;
}

void parse_component_line(component_t *component, char *line, int n) {
  int i = 1, command_start;
  command_t *c;

  while (line[i - 1] != '=' && line[i] != ' ') i++;
  i += 3;
  command_start = i;

  for (; i < n; i++) {
    if (line[i - 1] == ' ' && line[i] == '|') {
      c = parse_command_line(line + command_start, i - command_start - 1);
      component_append(component, c);
      command_start = i + 2;
    }
  }

  c = parse_command_line(line + command_start, i - command_start - 1);
  component_append(component, c);
}

void exec_pipeline(command_t *c) {
  int fd[2], in = -1, status;
  pid_t pid;

  for (; c != NULL; c = c->next) {
    if (pipe(fd) == -1) {
      printf("pipe failed.\n");
      return;
    }

    pid = fork();

    if (pid == 0) {
      close(fd[PIPE_READ]);

      if (in != -1) {
        dup2(in, STDIN_FILENO);
      }

      if (c->next != NULL) {
        dup2(fd[PIPE_WRITE], STDOUT_FILENO);
      }

      execvp(c->prog, c->argv);

      printf("FAILED!\n");
      _exit(1);
    }

    close(fd[PIPE_WRITE]);
    in = fd[PIPE_READ];

    wait(&status);
  }
}

void print_command(command_t *c) {
  printf("prog: %s\n", c->prog);
  for (int i = 0; i < ARGV_MAX_LENGTH; i++) {
    if (c->argv[i] == NULL) {
      printf("argv[%d] = NULL\n", i);
      break;
    }
    printf("argv[%d] = %s\n", i, c->argv[i]);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <path to file>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    printf("FIled to open file %s\n", argv[1]);
    return 1;
  }

  component_t components[COMPONENT_MAX_LENGTH] = {0};
  component_t merge;
  merge.head = NULL;

  char *line = NULL;
  size_t buff_size = 0;
  int n;
  int component_id = 0;

  int parse_component = 1;

  while ((n = getline(&line, &buff_size, f)) > 0) {
    if (line[0] == '\n') {
      parse_component = 0;
      continue;
    }

    if (parse_component) {
      parse_component_line(&components[component_id], line, n);
      component_id++;
      continue;
    }

    /* Parse pipeline. Only digit 0-9. */
    for (int i = 0; i < n; i++) {
      if (line[i] >= '0' && line[i] <= '9') {
        int id = line[i] - '0' - 1;
        component_merge(&merge, &components[id]);
      }
    }

    /* Accept only 1 pipeline. */
    break;
  }

  exec_pipeline(merge.head);

  free(line);
  return 0;
}

