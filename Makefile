CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -Werror=vla -ggdb
all:
	$(CC) $(CFLAGS) src/cco.c main.c -o main

clean:
	rm main
