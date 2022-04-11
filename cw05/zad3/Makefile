.PHONY: build clean

CC = gcc
CFLASG = -O2 -Wall -Wextra

build:
	$(CC) $(CFLASG) writer.c -o writer
	$(CC) $(CFLASG) reader.c -o reader

test:
	$(CC) $(CFLASG) tester.c -o tester
	./tester 6 1 100

run:
	./writer asdf 1 data/1.txt 33

clean:
	rm writer
	rm reader

