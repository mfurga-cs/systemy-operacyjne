#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "game.h"

#define MAX_CLIENTS 16
#define MESSAGE_TYPE_NICK 0
#define MESSAGE_TYPE_MOVE 1

#define POLLS_SIZE ((MAX_CLIENTS) + 1)

games_t games;
int last_fd;
char last_nick[GAME_MAX_NICK];

int listen_socket_tcp(int port) {

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    puts("Failed to create a socket.");
    return -1;
  }

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
      &opt, sizeof(opt)) == -1) {
    puts("setsockopt failed.");
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    puts("bind failed.");
    return -1;
  }

  if (listen(fd, 16) == -1) {
    puts("listen failed.");
    return -1;
  }

  return fd;
}

void close_connection(struct pollfd *polls, int fd) {
 for (int i = 0; i < POLLS_SIZE; i++) {
    if (polls[i].fd == fd) {
      polls[i].fd = 0;
      polls[i].events = 0;
      polls[i].revents = 0;
    }
  }
  //shutdown(fd, SHUT_RDWR);
  close(fd);
}

void handle_close(struct pollfd *polls, int fd) {
  if (fd == last_fd) {
    printf("Close connection (last): %d\n", fd);
    last_fd = -1;
    close_connection(polls, fd);
    return;
  }

  game_t *g = games_get(&games, fd);
  if (g == NULL) {
    return;
  }

  printf("Close connection: %d %d\n", g->player_id[0], g->player_id[1]);
  g->active = 0;

  int p1 = g->player_id[0];
  int p2 = g->player_id[1];

  close_connection(polls, p1);
  close_connection(polls, p2);
}

void handle_message_nick(struct pollfd *polls, char *nick, int fd) {
  char res[1024];
  int size;

  printf("New name: %s\n", nick);

  if (!games_nick_available(&games, nick)) {
    printf("%s exists 1.\n", nick);
    size = snprintf(res, sizeof(res),
      "The given nick already exists. Try another one.\n");
    send(fd, res, size, 0);
    //close(fd);
    close_connection(polls, fd);
    return;
  }

  if (last_fd == -1) {
    last_fd = fd;
    memcpy(last_nick, nick, GAME_MAX_NICK);
    size = snprintf(res, sizeof(res), "Waiting for opponent ...\n");
    send(fd, res, size, 0);
    return;
  }

  if (strcmp(last_nick, nick) == 0) {
    printf("%s exists 2.\n", nick);
    size = snprintf(res, sizeof(res),
      "The given nick already exists. Try another one.\n");
    send(fd, res, size, 0);
    //close(fd);
    close_connection(polls, fd);
    return;
  }

  game_t *g = games_add(&games, fd, last_fd, nick, last_nick);
  size = game_info(g, res, sizeof(res));

  send(fd, res, size, 0);
  send(last_fd, res, size, 0);

  last_fd = -1;
}

void handle_message_move(struct pollfd *polls, int move, int fd) {
  char res[1024];
  int size;

  game_t *g = games_get(&games, fd);
  assert(g != NULL);

  game_move(g, fd, move);
  size = game_info(g, res, sizeof(res));

  send(g->player_id[0], res, size, 0);
  send(g->player_id[1], res, size, 0);

  if (game_end(g)) {
    g->active = 0;
    close_connection(polls, g->player_id[0]);
    close_connection(polls, g->player_id[1]);
  }
}

void handle_connections(int server_tcp, int server_unix) {
  struct pollfd polls[POLLS_SIZE];
  memset(polls, 0, sizeof(polls));
  polls[0].fd = server_tcp;
  polls[0].events = POLLIN | POLLPRI;
  int conns = 0;

  while (1) {
    int rpoll = poll(polls, POLLS_SIZE, 3000);
    if (rpoll == 0) {
      continue;
    }

    if (polls[0].revents & POLLIN && conns < MAX_CLIENTS) {
      struct sockaddr_in client;
      socklen_t client_len = sizeof(client);
      int cfd = accept(server_tcp, (struct sockaddr *)&client, &client_len);

      printf("New TCP connection: %d\n", cfd);

      for (size_t i = 1; i < POLLS_SIZE; i++) {
        if (polls[i].fd == 0) {
          polls[i].fd = cfd;
          polls[i].events = POLLIN | POLLRDHUP;
          break;
        }
      }
      conns++;
    }

    for (size_t i = 1; i < POLLS_SIZE; i++) {
      if (polls[i].fd == 0) {
        continue;
      }

      if (polls[i].revents & POLLRDHUP) {
        handle_close(polls, polls[i].fd);
        conns--;
      }

      if (polls[i].revents & POLLIN) {
        char msg[1024];
        int size = read(polls[i].fd, msg, sizeof(msg));
        if (size == 0) {
          continue;
        }
        msg[size] = '\0';

        printf("New message: %s\n", msg + 1);

        switch (msg[0]) {
          case MESSAGE_TYPE_NICK:
            handle_message_nick(polls, msg + 1, polls[i].fd);
          break;
          case MESSAGE_TYPE_MOVE:
            handle_message_move(polls, msg[1] - '0', polls[i].fd);
          break;
          default:
            printf("%x command not supported.\n", msg[0]);
          break;
        }
      }
    }
  }
}

int main(int argc, char **argv) {

  if (argc != 3) {
    printf("Usage: %s <PORT> <PATH>\n", argv[0]);
    return 1;
  }

  int port = atoi(argv[1]);
  char *path = argv[2];

  printf("PID: %d\n", getpid());

  games_init(&games);
  last_fd = -1;

  printf("Init game server ...\n");

  int server_tcp = listen_socket_tcp(port);
  if (server_tcp == -1) {
    return 1;
  }

  printf("Listening on %d ...\n", port);

  handle_connections(server_tcp, 0);

  return 0;
}

