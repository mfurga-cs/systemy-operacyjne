#pragma once

#define CONNECTION_MAX_SIZE 256
#define MESSAGE_MAX_SIZE 512

#define SERVER_PROD_ID 1

#define MESSAGE_TYPE 0xff0000

#define MESSAGE_TYPE_STOP 0x10000L
#define MESSAGE_TYPE_LIST 0x20000L
#define MESSAGE_TYPE_INIT 0x30000L
#define MESSAGE_TYPE_2ONE 0x40000L
#define MESSAGE_TYPE_2ALL 0x50000L

typedef struct msgbuff {
  long mtype;
  char mtext[MESSAGE_MAX_SIZE];
} msgbuff_t;

int send_message(int qid, msgbuff_t *msg) {
  if (msgsnd(qid, msg, MESSAGE_MAX_SIZE, 0) == -1) {
    return 1;
  }
  return 0;
}

int recv_message(int qid, msgbuff_t *msg) {
  if (msgrcv(qid, msg, MESSAGE_MAX_SIZE, 0, 0) == -1) {
    return 1;
  }
  return 0;
}

