#include <stdio.h>
#include <stdlib.h>
#include "../../include/cco.h"

void worker(CCo_Channel *chan) {
    for (int i = 0; i < 1000; i++) {
        cco_send(chan, &i);
        if (i % 3 == 0) {
            cco_yield();
        }
    }
}

void cco_main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    CCo_Channel *data_chan = cco_make_chan(sizeof(int), 8);

    cco_run(worker, data_chan);

    for (int i = 0; i < 1000; i++) {
        int j;
        cco_recv(data_chan, &j);
        printf("Chan: %d\n", j);
    }

    cco_free_chan(data_chan);
}
