#include "../../include/cco.h"
#include <stdio.h>

void short_lived(void) {
    printf("Short: I only run once\n");
}

void long_lived(void) {
    for (int i = 0; i < 4; i++) {
        printf("Long: Step %d\n", i);
        cco_yield();
    }
}

void cco_main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    cco_run(short_lived, 0);
    cco_run(long_lived, 0);
}
