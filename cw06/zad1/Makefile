.PHONY: build clean

CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-format-truncation

build:
	$(CC) $(CFLAGS) server.c -o server
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm server
	rm client

