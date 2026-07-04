CFLAGS=-Wall -Wextra -std=c99 -pedantic -Werror=vla -fno-omit-frame-pointer -DCCO_THREADS_NUM=$(nproc) -ggdb
# -fno-omit-frame-pointer ensures that the compiler keeps the base stack pointer

TEST_SRC := $(wildcard test/*.c)

all: main test

main:
	$(CC) $(CFLAGS) src/cco.c src/cco_asm_procedures.s main.c -o main

test: $(TEST_SRC:.c=)
test/%: test/%.c
	$(CC) $(CFLAGS) src/cco.c src/cco_asm_procedures.s $< -o $@
	@bash test/run_test.sh $@
	@rm -f $@

clean:
	rm main
