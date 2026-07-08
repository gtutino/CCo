#include "../../include/cco.h"
#include <stdio.h>

void deep_func_level_2(int id) {
    printf("Deep Level 2 (Coro %d) - Before Yield\n", id);
    cco_yield();
    printf("Deep Level 2 (Coro %d) - After Yield\n", id);
}

void deep_func_level_1(int id) {
    printf("Deep Level 1 (Coro %d) - Entering\n", id);
    deep_func_level_2(id);
    printf("Deep Level 1 (Coro %d) - Exiting\n", id);
}

void standard_coro(int id) {
    printf("Standard Coro %d - Step 1\n", id);
    cco_yield();
    printf("Standard Coro %d - Step 2\n", id);
}

void cco_main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    cco_run(deep_func_level_1, 101);
    cco_run(standard_coro, 202);
}
