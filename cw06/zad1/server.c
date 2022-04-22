#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "config.h"

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
  printf("%d\n", msg->mtext[0]);

  if (send_message(conns[next_client_id], msg) != 0) {
    return;
  }

  next_client_id++;
}

void handle_stop(msgbuff_t *msg) {
  int client_id = msg->mtype & 0xff;
  if (client_id >= CONNECTION_MAX_SIZE) {
    return;
  }
  printf("HANDLE_STOP: %d\n", client_id);
  conns[client_id] = -1;
}

void handle_2one(msgbuff_t *msg) {
  int from = msg->mtype & 0xff;
  int to = (msg->mtype >> 8) & 0xff;

  msgbuff_t m;
  m.mtype = MESSAGE_TYPE_2ONE;
  snprintf(m.mtext, sizeof(m.mtext), "Message from %d: %s\n", from, msg->mtext);

  send_message(conns[to], &m);
}

void handle_2all(msgbuff_t *msg) {
  int from = msg->mtype & 0xff;

  msgbuff_t m;
  m.mtype = MESSAGE_TYPE_2ALL;
  snprintf(m.mtext, sizeof(m.mtext), "Message from %d: %s\n", from, msg->mtext);

  for (int i = 0; MESSAGE_MAX_SIZE; i++) {
    if (conns[i] == -1) {
      continue;
    }
    send_message(conns[i], &m);
  }
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

  int qid = msgget(key, IPC_CREAT | 0777);
  if (qid == -1) {
    perror("msgget failed.");
    return 1;
  }

  printf("Server created. ID = %d\n", qid);

  msgbuff_t msg;
  while (msgrcv(qid, &msg, sizeof(msg.mtext), 0, 0) != -1) {
    printf("Recv(%ld): %s\n", msg.mtype, msg.mtext);

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
      default:
        puts("wrong mtype");
      break;
    }
  }

  return 0;
}

