#include "common.h"
#include <stdlib.h>
#include <stdio.h>

// OOM is handle by simply aborting
void *cco_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR]: OOM error when allocating %zu bytes, aborting.\n", size);
        exit(1);
    }
    return ptr;
}
