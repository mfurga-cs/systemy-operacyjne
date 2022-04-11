.PHONY: build clean

CC = gcc
CFLASG = -O2 -Wall -Wextra

build:
	$(CC) $(CFLASG) writer.c -o writer
	$(CC) $(CFLASG) reader.c -o reader
	$(CC) $(CFLASG) tester.c -o tester

test: build
	./tester 1 10 500
	python3 tester.py

clean:
	rm writer
	rm reader
	rm tester
	rm fifo

