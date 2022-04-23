#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "config.h"

#define CMP4(s1, s2) \
  s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2] && s1[3] == s2[3]

int sid;
int cid;
int client_id;
pid_t child;

int send_message_init(int data) {
  msgbuff_t msg;
  msg.mtype = MESSAGE_TYPE_INIT;

  msg.mtext[0] = (data >> 0)  & 0xff;
  msg.mtext[1] = (data >> 8)  & 0xff;
  msg.mtext[2] = (data >> 16) & 0xff;
  msg.mtext[3] = (data >> 24) & 0xff;

  if (send_message(sid, &msg) != 0) {
    return 1;
  }

  if (recv_message(cid, &msg) != 0) {
    puts("recv error");
    return 1;
  }

  client_id = msg.mtext[0];
  return 0;
}

void handle_list() {
  msgbuff_t msg;
  msg.mtype = MESSAGE_TYPE_LIST | (client_id & 0xff);
  send_message(sid, &msg);
}

void handle_2one(char *line) {
  int to = 0;
  int i = 0;

  for (i = 0; line[i] != ' '; i++) {
    to *= 10;
    to += line[i] - '0';
  }
  i++;

  msgbuff_t msg;
  msg.mtype = MESSAGE_TYPE_2ONE | ((to & 0xff) << 8) | (client_id & 0xff);
  snprintf(msg.mtext, sizeof(msg.mtext), "%s", line + i);

  send_message(sid, &msg);
}

void handle_2all(char *line) {
  msgbuff_t msg;
  msg.mtype = MESSAGE_TYPE_2ALL | (client_id & 0xff);
  snprintf(msg.mtext, sizeof(msg.mtext), "%s", line);
  send_message(sid, &msg);
}

void handle_stop(void) {
  msgbuff_t msg;
  msg.mtype = MESSAGE_TYPE_STOP | (client_id & 0xff);
  send_message(sid, &msg);
  msgctl(cid, IPC_RMID, NULL);

  kill(child, 9);
  _exit(0);
}

int init_connection() {
  char *home = getenv("HOME");
  if (home == NULL) {
    puts("$HOME not declared.");
    return 1;
  }

  key_t skey = ftok(home, SERVER_PROD_ID);
  sid = msgget(skey, 0777);
  if (sid == -1) {
    return 1;
  }

  key_t ckey = ftok(home, rand());
  cid = msgget(ckey, IPC_CREAT | 0777);
  if (cid == -1) {
    return 1;
  }

  return send_message_init((int)ckey);
}

void handler_SIGINT(int signum) {
  (void)signum;
  handle_stop();
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  srand(time(NULL));

  if (init_connection() != 0) {
    puts("init connection failed.");
  }

  printf("Your client ID: %d\n", client_id);

  child = fork();
  if (child == 0) {
    msgbuff_t msg;
    while (msgrcv(cid, &msg, sizeof(msg.mtext), -MESSAGE_TYPE_MAX, 0) != -1) {
      if ((msg.mtype & MESSAGE_TYPE) == MESSAGE_TYPE_STOP) {
        printf("[SERVER] Dead.\n");
        kill(getppid(), SIGINT);
        return 0;
      }
      printf("[SERVER] %s> ", msg.mtext);
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

  return 0;
}

