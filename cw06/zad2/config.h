#pragma once

#include <string.h>

#define CONNECTION_MAX_SIZE 256
#define MESSAGE_MAX_SIZE 512
#define CLIENTS_MAX_SIZE 128

#define SERVER_NAME "/server"

#define QUEUE_MESSAGE_MAX_SIZE 512
#define QUEUE_MESSAGE_MAX 4

#define MESSAGE_TYPE_INIT 1
#define MESSAGE_TYPE_2ONE 2
#define MESSAGE_TYPE_2ALL 3
#define MESSAGE_TYPE_LIST 10
#define MESSAGE_TYPE_STOP 11

int queue_init(const char *name) {
  struct mq_attr attr = {
    .mq_maxmsg = QUEUE_MESSAGE_MAX,
    .mq_msgsize = QUEUE_MESSAGE_MAX_SIZE
  };
  return mq_open(name, O_RDONLY | O_CREAT, 0777, &attr);
}

int queue_remove(const char *name) {
  return mq_unlink(name);
}

int send_message(mqd_t id, int type, char *msg, int len) {
  char buff[len + 1];
  buff[0] = (char)type;
  memcpy(buff + 1, msg, len);
  return mq_send(id, buff, sizeof(buff), type);
}

int recv_message(mqd_t id, int *type, char *msg, int len, unsigned *prio) {
  char buff[len + 1];
  int read = mq_receive(id, buff, sizeof(buff), prio);
  if (read == -1) {
    return -1;
  }
  *type = buff[0];
  memcpy(msg, buff + 1, len);
  return read - 1;
}

