#include "../../include/cco.h"
#include <stdio.h>

void worker(int id, int max_count) {
    for (int i = 1; i <= max_count; i++) {
        printf("Worker %d: count %d\n", id, i);
        cco_yield();
    }
}

void cco_main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    cco_run(worker, 1, 3);
    cco_run(worker, 2, 2);
}
