#include "../../include/cco.h"
#include <stdio.h>

void worker(int id, int max_count) {
    for (int i = 1; i <= max_count; i++) {
        printf("Worker %d: count %d\n", id, i);
        cco_yield();
    }
}

void cco_main(void) {
    cco_run(worker, 2, 1, 3);
    cco_run(worker, 2, 2, 2);
}

int main(void) {
    cco_run(cco_main, 0, 0);
}
