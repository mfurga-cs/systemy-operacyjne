.PHONY: build clean

CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-format-truncation

build:
	$(CC) $(CFLAGS) server.c -o server -lrt
	$(CC) $(CFLAGS) client.c -o client -lrt

clean:
	rm server
	rm client

