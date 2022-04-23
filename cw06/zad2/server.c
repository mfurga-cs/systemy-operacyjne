#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <time.h>
#include <assert.h>

#include "config.h"

mqd_t sid;
mqd_t conns[CLIENTS_MAX_SIZE] = { -1 };

int next_client_id = 0;

void logging(int type, const char *msg) {
  FILE *f = fopen("log.txt", "a");
  if (f == NULL) {
    return;
  }

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  int client_id = msg[0];

  char buff[strlen(msg) + 128];
  int p = strftime(buff, sizeof(buff), "[%c] ", tm);
  p += snprintf(buff + p, sizeof(buff) - p, "[client %d] [type %d] %s\n", client_id, type, msg + 1);

  fwrite(buff, 1, p, f);
  fclose(f);
}

void handle_init(char *msg, int size) {
  (void)size;

  conns[next_client_id] = mq_open(msg, O_WRONLY);
  if (conns[next_client_id] == -1) {
    perror("open client failed.");
    return;
  }

  char client_id[1];
  client_id[0] = (char)next_client_id;
  printf("New connection: %d\n", next_client_id);

  if (send_message(conns[next_client_id], MESSAGE_TYPE_INIT,
                   client_id, sizeof(client_id)) == -1) {
    perror("send_message failed.");
    return;
  }
  next_client_id++;
}

void handle_list(const char *msg, int size) {
  (void)size;

  int client_id = msg[0];

  char buff[64];
  int p = snprintf(buff, sizeof(buff), "Clients list: ");

  for (int i = 0; i < next_client_id; i++) {
    if (conns[i] == -1) {
      continue;
    }
    p += snprintf(buff + p, sizeof(buff) - p, "%d ", i);
  }
  buff[p] = '\0';

  send_message(conns[client_id], MESSAGE_TYPE_LIST, buff, sizeof(buff));
}

void handle_stop(const char *msg, int size) {
  (void)size;

  int client_id = msg[0];

  if (client_id >= next_client_id) {
    return;
  }

  if (conns[client_id] == -1) {
    return;
  }

  printf("Close connection: %d\n", client_id);

  mq_close(conns[client_id]);
  conns[client_id] = -1;
}

void handle_2one(const char *msg, int size) {
  int from = msg[0];
  int to = msg[1];

  if (from >= next_client_id) {
    return;
  }

  if (conns[from] == -1) {
    return;
  }

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char buff[size + 128];
  int p = strftime(buff, sizeof(buff), "[%c] ", tm);
  snprintf(buff + p, sizeof(buff) - p, "message from client %d: %s", from, msg + 2);

  send_message(conns[to], MESSAGE_TYPE_2ONE, buff, sizeof(buff));
}

void handle_2all(const char *msg, int size) {
  int from = msg[0];

  if (from >= next_client_id) {
    return;
  }

  if (conns[from] == -1) {
    return;
  }

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char buff[size + 128];
  int p = strftime(buff, sizeof(buff), "[%c] ", tm);
  snprintf(buff + p, sizeof(buff) - p, "message from client %d: %s", from, msg + 1);

  for (int i = 0; i < next_client_id; i++) {
    if (conns[i] == -1) {
      continue;
    }
    send_message(conns[i], MESSAGE_TYPE_2ALL, buff, sizeof(buff));
  }
}

void close_server(void) {
  char buff[1];

  for (int i = 0; i < next_client_id; i++) {
    if (conns[i] == -1) {
      continue;
    }
    send_message(conns[i], MESSAGE_TYPE_STOP, buff, sizeof(buff));
    mq_close(conns[i]);
    conns[i] = -1;
  }

  queue_remove(SERVER_NAME);
  _exit(0);
}

void handler_SIGINT(int signum) {
  (void)signum;
  close_server();
}

int main(void) {
  puts("Staring server ...");

  sid = queue_init(SERVER_NAME);
  if (sid == -1) {
    perror("mq_open failed.");
    return 1;
  }

  printf("mp_open created: %d\n", sid);

  signal(SIGINT, handler_SIGINT);

  int read, type;
  unsigned prio;
  char msg[QUEUE_MESSAGE_MAX_SIZE];

  while ((read = recv_message(sid, &type, msg, sizeof(msg), &prio)) != -1) {

    logging(type, msg);

    switch (type) {
      case MESSAGE_TYPE_INIT:
        handle_init(msg, read);
      break;
      case MESSAGE_TYPE_STOP:
        handle_stop(msg, read);
      break;
      case MESSAGE_TYPE_2ONE:
        handle_2one(msg, read);
      break;
      case MESSAGE_TYPE_2ALL:
        handle_2all(msg, read);
      break;
      case MESSAGE_TYPE_LIST:
        handle_list(msg, read);
      break;
      default:
        puts("wrong mtype");
      break;
    }
  }

  if (mq_close(sid) == -1) {
    perror("mq_close failed.");
    return 1;
  }

  if (queue_remove(SERVER_NAME) == -1) {
    perror("mq_close failed.");
    return 1;
  }

  return 0;
}

