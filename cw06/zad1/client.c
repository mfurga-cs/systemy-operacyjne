#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "config.h"

int sid;
int cid;
int client_id;

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

int handle_close(void) {
  msgbuff_t msg;
  msg.mtype = MESSAGE_TYPE_STOP | (client_id & 0xff);
  if (send_message(sid, &msg) != 0) {
    return 1;
  }
  if (msgctl(cid, IPC_RMID, NULL) != 0) {
    return 1;
  }
  return 0;
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

int main(void) {
  srand(time(NULL));

  if (init_connection() != 0) {
    puts("init connection failed.");
  }

  printf("OK\n");
  printf("sid: %d\n", sid);
  printf("cid: %d\n", cid);
  printf("client_id: %d\n", client_id);

  char *line;
  while ((line = readline("> ")) != NULL) {
    printf("%s\n", line);
  }

  if (handle_close() != 0) {
    puts("handle_close failed.");
  }

  return 0;
}

