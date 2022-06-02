#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <poll.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "game.h"

#define MESSAGE_TYPE_NICK 0
#define MESSAGE_TYPE_MOVE 1

int connect_socket_udp(int port) {

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) {
    puts("Failed to create a socket.");
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
    puts("inet_pton failed.");
    return -1;
  }

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    puts("connect failed.");
    return -1;
  }

  return fd;
}

int connect_socket_unix(char *path) {

  int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (fd == -1) {
    puts("Failed to create a socket.");
    return -1;
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path),
    "/tmp/%ld%d", time(NULL), rand());

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    puts("bind failed.");
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, path);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("error: ");
    puts("connect failed.");
    return -1;
  }

  return fd;
}

int main(int argc, char **argv) {

  if (argc != 4) {
    printf("Usage: %s <NICK> <UDP/UNIX> <PORT/PATH>\n", argv[0]);
    return 1;
  }

  srand(time(NULL));


  int fd;
  char *nick = argv[1];

  if (strcmp(argv[2], "UDP") == 0) {
    int port = atoi(argv[3]);
    fd = connect_socket_udp(port);
  } else if (strcmp(argv[2], "UNIX") == 0) {
    fd = connect_socket_unix(argv[3]);
  } else {
    printf("Invalid connection type.\n");
    return 1;
  }

  if (fd == -1) {
    return 1;
  }

  char msg[1024];
  msg[0] = '\0';
  memcpy(msg + 1, nick, strlen(nick));
  send(fd, msg, strlen(nick) + 1, 0);

  struct pollfd polls[2];
  memset(polls, 0, sizeof(polls));

  polls[0].fd = 0;
  polls[0].events = POLLIN | POLLPRI;

  polls[1].fd = fd;
  polls[1].events = POLLIN | POLLRDHUP;

  int size;
  while (1) {
    int rpoll = poll(polls, 2, 3000);
    if (rpoll == 0) {
      continue;
    }

    if (polls[0].revents & POLLIN) {
      size = read(0, msg, sizeof(msg));
      if (size > 0) {
        msg[1] = msg[0];
        msg[0] = MESSAGE_TYPE_MOVE;
        size = write(fd, msg, 2);
      }
    }

    if (polls[1].revents & POLLIN) {
      size = read(fd, msg, sizeof(msg));
      if (msg[0] == 0) {
        return 0;
      }
      msg[size] = '\0';
      printf("%s", msg);
    }

    if (polls[1].revents & POLLRDHUP) {
      break;
    }
  }

  shutdown(fd, SHUT_RDWR);
  close(fd);
  return 0;
}

