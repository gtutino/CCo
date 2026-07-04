#include "../include/cco.h"
#include <stdio.h>
#include <stdlib.h>

void c1(void) {
    for (int i = 0; i < 10; i++) {
        printf("C1 : %d\n", i);
        cco_yield();
    }
}

void c2(void) {
    for (int i = 0; i < 10; i++) {
        printf("C2 : %d\n", i);
        cco_yield();
    }
}

void cco_main(void) {
    cco_run(c1, 0, 0);
    cco_run(c2, 0, 0);
}

int main(void) {
    cco_run(cco_main, 0, 0);
}
