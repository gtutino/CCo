CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -Werror=vla -ggdb
all:
	$(CC) $(CFLAGS) src/cco.c src/cco_save_ctx.s src/cco_yield_run_next.s main.c -o main

clean:
	rm main
