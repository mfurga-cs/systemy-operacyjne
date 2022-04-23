#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <time.h>

#include "config.h"

#define CMP4(s1, s2) \
  s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3]

mqd_t sid;
mqd_t cid;
char cname[16];
int client_id;
pid_t child;

void handle_list(void) {
  char buff[1] = { client_id };
  send_message(sid, MESSAGE_TYPE_LIST, buff, sizeof(buff));
}

void handle_stop(void) {
  char buff[1] = { client_id };
  send_message(sid, MESSAGE_TYPE_STOP, buff, sizeof(buff));

  mq_close(sid);
  mq_close(cid);
  queue_remove(cname);

  kill(child, 9);
  _exit(0);
}

void handle_2all(char *line) {
  int size = strlen(line);
  char buff[size + 1];
  buff[0] = client_id;
  snprintf(buff + 1, size, "%s", line);

  send_message(sid, MESSAGE_TYPE_2ALL, buff, sizeof(buff));
}

void handle_2one(char *line) {
  int to = 0;
  int i = 0;

  for (i = 0; line[i] != ' '; i++) {
    to *= 10;
    to += line[i] - '0';
  }
  i++;

  int size = strlen(line + i);
  char buff[size + 2];
  buff[0] = client_id;
  buff[1] = to;
  snprintf(buff + 2, size, "%s", line + i);
  send_message(sid, MESSAGE_TYPE_2ONE, buff, sizeof(buff));
}

int init_connection(void) {
  if (send_message(sid, MESSAGE_TYPE_INIT, cname, sizeof(cname)) == -1) {
    perror("send_message failed.");
    return -1;
  }

  char buff[QUEUE_MESSAGE_MAX_SIZE];
  int type;

  if (recv_message(cid, &type, buff, sizeof(buff), NULL) == -1) {
    perror("recv_message failed.");
    return -1;
  }

  client_id = buff[0];
  return 0;
}

void handler_SIGINT(int signum) {
  (void)signum;
  handle_stop();
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);

  // Open server queue.
  sid = mq_open(SERVER_NAME, O_WRONLY);
  if (sid == -1) {
    perror("mq_open failed.");
    return 1;
  }

  printf("Server id: %d\n", sid);

  // Create client queue.
  snprintf(cname, sizeof(cname), "/%d", getpid());
  cid = queue_init(cname);
  if (cid == -1) {
    perror("queue_init failed.");
    return 1;
  }

  if (init_connection() == -1) {
    perror("init_connection failed.");
    return 1;
  }

  printf("Client id: %d\n", client_id);

  child = fork();
  if (child == 0) {
    char msg[QUEUE_MESSAGE_MAX_SIZE];
    int read, type;

    while ((read = recv_message(cid, &type, msg, sizeof(msg), NULL)) != -1) {
      if (type == MESSAGE_TYPE_STOP) {
        printf("[SERVER] Dead.\n");
        kill(getppid(), SIGINT);
        return 0;
      }
      printf("[SERVER] %s\n> ", msg);
    }
    return 0;
  }

  signal(SIGINT, handler_SIGINT);

  char *line = NULL;
  size_t len = 0;
  printf("> ");
  while (getline(&line, &len, stdin) > 0) {
    if (CMP4(line, "LIST")) {
      handle_list();
    } else if (CMP4(line, "2ALL")) {
      handle_2all(line + 5);
    } else if (CMP4(line, "2ONE")) {
      handle_2one(line + 5);
    } else if (CMP4(line, "STOP")) {
      handle_stop();
    } else {
      puts("wrong command");
    }
    printf("> ");
  }

  free(line);


  if (mq_close(sid) == -1) {
    perror("mq_close failed.");
    return 1;
  }

  if (mq_close(cid) == -1) {
    perror("mq_close failed.");
    return 1;
  }

  if (queue_remove(cname) == -1) {
    perror("mq_close failed.");
    return 1;
  }

  return 0;
}

