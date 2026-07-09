CFLAGS=-Wall -Wextra -std=c11 -pedantic -Werror=vla -fno-omit-frame-pointer -ggdb -pthread
# -fno-omit-frame-pointer ensures that the compiler keeps the base stack pointer

CCO_SRC := $(wildcard src/*.c)
CCO_OBJ := $(patsubst src/%.c, src/%.o, $(CCO_SRC)) src/cco_asm_procedures.o

all: main test


main: $(CCO_OBJ) main.c
	$(CC) $(CFLAGS) -O3 $(CCO_OBJ) main.c -o main

src/%.o: src/%.c
	$(CC) $(CFLAGS) -O0 -c $< -o $@

src/%.o: src/%.s
	$(CC) $(CFLAGS) -O0 -c $< -o $@


cco.a: $(CCO_OBJ)
	ar rcs cco.a $(CCO_OBJ)


TEST_SRC := $(wildcard test/src/*.c)
TEST_BIN := $(TEST_SRC:.c=)

test: $(TEST_BIN)
test/%: test/%.c
	@$(CC) $(CFLAGS) -Wno-pedantic -DCCO_THREAD_NUM=1 src/coroutine.c src/common.c src/channel.c src/cco_asm_procedures.s $< -o $@
	@bash test/run_tests.sh $(notdir $@)


clean:
	rm -f main $(CCO_OBJ) $(TEST_BIN) cco.a
