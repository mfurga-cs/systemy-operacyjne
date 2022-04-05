.PHONY: build clean

CC = gcc
CFLASG = -O2 -Wall -Wextra

build:
	$(CC) $(CFLASG) sender.c -o sender
	$(CC) $(CFLASG) catcher.c -o catcher

clean:
	rm sender
	rm catcher

