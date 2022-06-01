.PHONY: build clean

CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-format-truncation -pthread

build:
	$(CC) $(CFLAGS) server.c game.c -o server
	$(CC) $(CFLAGS) client.c game.c -o client

clean:
	rm server
	rm client

