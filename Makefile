CFLAGS=-Wall -Wextra -std=c99 -pedantic -Werror=vla -fno-omit-frame-pointer -DCCO_THREADS_NUM=$(nproc) -ggdb
# -fno-omit-frame-pointer ensures that the compiler keeps the base stack pointer

all: main test

main:
	$(CC) $(CFLAGS) src/coroutine.c src/common.c src/cco_asm_procedures.s main.c -o main


TEST_SRC := $(wildcard test/src/*.c)
TEST_BIN := $(TEST_SRC:.c=)

test: $(TEST_BIN)
test/%: test/%.c
	@$(CC) $(CFLAGS) src/coroutine.c src/common.c src/cco_asm_procedures.s $< -o $@
	@bash test/run_tests.sh $(notdir $@)
	@rm -f $@

clean:
	rm main
