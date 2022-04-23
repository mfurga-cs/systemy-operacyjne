#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

#include "config.h"

int sid;
int next_client_id = 1;
int conns[CONNECTION_MAX_SIZE] = { -1 };

void handle_init(msgbuff_t *msg) {
  if (next_client_id >= CONNECTION_MAX_SIZE) {
    return;
  }

  key_t qkey = (msg->mtext[3] << 24) |
               (msg->mtext[2] << 16) |
               (msg->mtext[1] << 8 ) |
               (msg->mtext[0] << 0 );

  conns[next_client_id] = msgget(qkey, 0777);
  if (conns[next_client_id] == -1) {
    return;
  }

  msg->mtext[0] = (char)next_client_id;
  printf("New client: %d\n", msg->mtext[0]);

  if (send_message(conns[next_client_id], msg) != 0) {
    return;
  }

  next_client_id++;
}

void handle_stop(msgbuff_t *msg) {
  int client_id = msg->mtype & 0xff;
  if (client_id >= next_client_id) {
    return;
  }
  conns[client_id] = -1;
  printf("Close connection: %d\n", client_id);
}

void handle_list(msgbuff_t *msg) {
  int client_id = msg->mtype & 0xff;

  msgbuff_t m;
  m.mtype = MESSAGE_TYPE_LIST;
  int p = snprintf(m.mtext, sizeof(m.mtext), "Clients list: ");

  for (int i = 0; i < next_client_id; i++) {
    if (conns[i] == -1) {
      continue;
    }
    p += snprintf(m.mtext + p, sizeof(m.mtext) - p, "%d ", i);
  }
  m.mtext[p] = '\n';
  m.mtext[p + 1] = '\0';
  send_message(conns[client_id], &m);
}

void handle_2one(msgbuff_t *msg) {
  int from = msg->mtype & 0xff;
  int to = (msg->mtype >> 8) & 0xff;

  if (to >= next_client_id) {
    return;
  }

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);

  msgbuff_t m;
  m.mtype = MESSAGE_TYPE_2ONE;

  int p = strftime(m.mtext, sizeof(m.mtext), "[%c] ", tm);
  snprintf(m.mtext + p, sizeof(m.mtext) - p, "message from client %d: %s", from, msg->mtext);

  send_message(conns[to], &m);
}

void handle_2all(msgbuff_t *msg) {
  int from = msg->mtype & 0xff;

  msgbuff_t m;
  m.mtype = MESSAGE_TYPE_2ALL;
  snprintf(m.mtext, sizeof(m.mtext), "message from client %d: %s", from, msg->mtext);

  for (int i = 0; i < next_client_id; i++) {
    if (conns[i] == -1) {
      continue;
    }
    send_message(conns[i], &m);
  }
}

void close_server() {
  msgbuff_t m;
  m.mtype = MESSAGE_TYPE_STOP;

  for (int i = 0; i < next_client_id; i++) {
    if (conns[i] == -1) {
      continue;
    }
    send_message(conns[i], &m);
  }

  msgctl(sid, IPC_RMID, NULL);
}

void handler_SIGINT(int signum) {
  (void)signum;
  close_server();
}

int main(void) {
  puts("Staring server ...");

  char *home = getenv("HOME");

  if (home == NULL) {
    puts("$HOME not declared.");
    return 1;
  }
  printf("Read $HOME = %s\n", home);

  key_t key = ftok(home, SERVER_PROD_ID);
  printf("Creating queue with key = %d\n", key);

  sid = msgget(key, IPC_CREAT | 0777);
  if (sid == -1) {
    perror("msgget failed.");
    return 1;
  }

  printf("Server created. ID = %d\n", sid);

  signal(SIGINT, handler_SIGINT);

  msgbuff_t msg;
  while (msgrcv(sid, &msg, sizeof(msg.mtext), -MESSAGE_TYPE_MAX, 0) != -1) {
    switch (msg.mtype & MESSAGE_TYPE) {
      case MESSAGE_TYPE_INIT:
        handle_init(&msg);
      break;
      case MESSAGE_TYPE_STOP:
        handle_stop(&msg);
      break;
      case MESSAGE_TYPE_2ONE:
        handle_2one(&msg);
      break;
      case MESSAGE_TYPE_2ALL:
        handle_2all(&msg);
      break;
      case MESSAGE_TYPE_LIST:
        handle_list(&msg);
      break;
      default:
        puts("wrong mtype");
      break;
    }
  }

  return 0;
}

