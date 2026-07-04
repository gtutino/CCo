CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -Werror=vla -fno-omit-frame-pointer -ggdb
# -fno-omit-frame-pointer ensure that the compiler keeps the base stack pointer
all:
	$(CC) $(CFLAGS) -DCCO_THREADS_NUM=$(nproc) src/cco.c src/cco_asm_procedures.s main.c -o main

clean:
	rm main
