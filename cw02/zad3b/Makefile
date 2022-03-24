.PHONY: build clean

CC = gcc
CFLASG = -O2 -Wall -Wextra -std=c99

BUILD_NAME = prog

SRC = zad3b.c

build:
	$(CC) $(CFLASG) $(SRC) -o $(BUILD_NAME)

clean: $(BUILD_NAME)
	rm $(BUILD_NAME)

