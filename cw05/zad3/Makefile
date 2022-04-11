.PHONY: build clean

CC = gcc
CFLASG = -O2 -Wall -Wextra

build:
	$(CC) $(CFLASG) writer.c -o writer
	$(CC) $(CFLASG) reader.c -o reader

run:
	./writer asdf 1 data/1.txt 3

clean:
	rm writer
	rm reader

