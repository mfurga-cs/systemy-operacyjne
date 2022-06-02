#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "game.h"

#define MAX_CLIENTS 16
#define MESSAGE_TYPE_NICK 0
#define MESSAGE_TYPE_MOVE 1

#define POLLS_SIZE 2

games_t games;
connection_t *last_id;
char last_nick[GAME_MAX_NICK];

int server_udp;
int server_unix;

connection_t active_clients[MAX_CLIENTS];

connection_t *clients_in(connection_t *conn) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (connection_cmp(&active_clients[i], conn)) {
      return &active_clients;
    }
  }
  return NULL;
}

void conn_sendto(connection_t *conn, char *mess, size_t size) {
  assert(conn->type != CONNECTION_NONE);

  if (conn->type == CONNECTION_UDP) {
    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));

    dst.sin_family = AF_INET;
    if (inet_pton(AF_INET, "127.0.0.1", &dst.sin_addr) <= 0) {
      puts("inet_pton failed.");
      return;
    }
    dst.sin_port = conn->conn.port;

    sendto(server_udp, mess, size, MSG_DONTWAIT,
           (struct sockaddr *)&dst, sizeof(dst));
  } else {
    struct sockaddr_un dst;
    memset(&dst, 0, sizeof(dst));

    dst.sun_family = AF_UNIX;
    strcpy(dst.sun_path, conn->conn.path);

    sendto(server_unix, mess, size, MSG_DONTWAIT,
           (struct sockaddr *)&dst, sizeof(dst));
  }
}

void clients_add(connection_t *conn) {
  assert(conn->type != CONNECTION_NONE);

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (active_clients[i].type == CONNECTION_NONE) {
      active_clients[i].type = conn->type;

      if (conn->type == CONNECTION_UDP) {
        active_clients[i].conn.port = conn->conn.port;
      } else {
        memset(active_clients[i].conn.path, 0, 108);
        strcpy(active_clients[i].conn.path, conn->conn.path);
      }
      break;
    }
  }
}

void clients_remove(connection_t *conn) {
  assert(conn->type != CONNECTION_NONE);
  char mess[1] = { 0 };
  conn_sendto(conn, mess, 1);

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (connection_cmp(&active_clients[i], conn)) {
      active_clients[i].type = CONNECTION_NONE;
      break;
    }
  }
}

int listen_socket_udp(int port) {

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
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

  return fd;
}

int listen_socket_unix(char *path) {

  int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
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

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, path, strlen(path));

  unlink(path);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    puts("bind failed.");
    return -1;
  }

  return fd;
}

/*
void handle_close(int id) {
  if (id == last_id) {
    printf("Close connection (last): %d\n", id);
    last_id = -1;
    clients_remove(id);
    return;
  }

  game_t *g = games_get(&games, id);
  if (g == NULL) {
    return;
  }

  printf("Close connection: %d %d\n", g->player_id[0], g->player_id[1]);
  g->active = 0;

  int p1 = g->player_id[0];
  int p2 = g->player_id[1];

  clients_remove(p1);
  clients_remove(p2);
}
*/

void handle_message_nick(char *nick, connection_t *conn) {
  char res[1024];
  int size;

  assert(conn->type != CONNECTION_NONE);

  if (!games_nick_available(&games, nick)) {
    printf("%s exists 1.\n", nick);
    size = snprintf(res, sizeof(res),
      "The given nick already exists. Try another one.\n");

    conn_sendto(conn, res, size);
    clients_remove(conn);
    return;
  }

  if (last_id == NULL) {
    last_id = conn;
    memcpy(last_nick, nick, GAME_MAX_NICK);
    size = snprintf(res, sizeof(res), "Waiting for opponent ...\n");

    conn_sendto(conn, res, size);
    return;
  }

  if (strcmp(last_nick, nick) == 0) {
    printf("%s exists 2.\n", nick);
    size = snprintf(res, sizeof(res),
      "The given nick already exists. Try another one.\n");

    conn_sendto(conn, res, size);
    clients_remove(conn);
    return;
  }

  game_t *g = games_add(&games, conn, last_id, nick, last_nick);
  size = game_info(g, res, sizeof(res));

  conn_sendto(g->player_id[0], res, size);
  conn_sendto(g->player_id[1], res, size);

  last_id = NULL;
}

void handle_message_move(int move, connection_t *conn) {
  char res[1024];
  int size;

  game_t *g = games_get(&games, conn);
  if (g == NULL) {
    return;
  }

  game_move(g, conn, move);
  size = game_info(g, res, sizeof(res));

  conn_sendto(g->player_id[0], res, size);
  conn_sendto(g->player_id[1], res, size);

  if (game_end(g)) {
    g->active = 0;
    clients_remove(g->player_id[0]);
    clients_remove(g->player_id[1]);
  }
}

void handle_connections() {
  struct pollfd polls[POLLS_SIZE];
  memset(polls, 0, sizeof(polls));
  polls[0].fd = server_udp;
  polls[0].events = POLLIN | POLLPRI;
  polls[1].fd = server_unix;
  polls[1].events = POLLIN | POLLPRI;

  int conns = 0;

  char msg[1024];
  while (1) {
    int rpoll = poll(polls, POLLS_SIZE, 3000);
    if (rpoll == 0) {
      continue;
    }

    if (polls[0].revents & POLLIN) {
      struct sockaddr_in client;
      socklen_t client_len = sizeof(client);

      int r = recvfrom(server_udp, msg, sizeof(msg),
        MSG_DONTWAIT, (struct sockaddr *)&client, &client_len);
      if (r <= 0) {
        continue;
      }
      msg[r] = 0;

      connection_t conn;
      conn.type = CONNECTION_UDP;
      conn.conn.port = client.sin_port;

      if (clients_in(&conn) == NULL) {
        clients_add(&conn);
        conns++;
      }

      switch (msg[0]) {
        case MESSAGE_TYPE_NICK:
          handle_message_nick(msg + 1, clients_in(&conn));
        break;
        case MESSAGE_TYPE_MOVE:
          handle_message_move(msg[1] - '0', clients_in(&conn));
        break;
        default:
          printf("%x command not supported.\n", msg[0]);
        break;
      }
    }

    if (polls[1].revents & POLLIN) {
      struct sockaddr_un client;
      socklen_t client_len = sizeof(client);
      int r = recvfrom(server_unix, msg, sizeof(msg),
        MSG_DONTWAIT, (struct sockaddr *)&client, &client_len);

      printf("New UNIX connection: %s\n", client.sun_path);

      char res[16];
      strcpy(res, "Hello :)");
/*
      connection_t conn;
      conn.type = CONNECTION_UDP;
      conn.conn.port = client.sin_port;

      if (!clients_in(&conn)) {
        clients_add(&conn);
        conns++;
      }

      switch (msg[0]) {
        case MESSAGE_TYPE_NICK:
          handle_message_nick(msg + 1, &conn);
        break;
        case MESSAGE_TYPE_MOVE:
          handle_message_move(msg[1] - '0', &conn);
        break;
        default:
          printf("%x command not supported.\n", msg[0]);
        break;
      }
 
      sendto(server_unix, res, sizeof(res), MSG_DONTWAIT,
         (struct sockaddr *)&client, sizeof(client));
      conns++;
      */
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

  memset(active_clients, 0, sizeof(active_clients));
  games_init(&games);
  last_id = NULL;

  printf("Init game server ...\n");

  server_udp = listen_socket_udp(port);
  server_unix = listen_socket_unix(path);
  if (server_udp == -1 || server_unix == -1) {
    return 1;
  }

  printf("Listening on %d ...\n", port);

  handle_connections();

  return 0;
}

