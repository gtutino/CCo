#include <stdio.h>
#include <stdlib.h>
#include "include/cco.h"

void worker(CCo_Channel *chan, int x) {
    for (int i = x; i < x + 10; i++) {
        cco_send(chan, &i);
    }
}

void reciver(CCo_Channel *data_chan1, CCo_Channel *data_chan2, CCo_Channel *done_chan) {
    for (int i = 0; i < 10000; i++) {
        int j;
        size_t chan = cco_select(
            cco_recv, data_chan1, &j,
            cco_recv, data_chan2, &j
        );
        if (chan != 0) {
            printf("Recived %d on channel %zu\n", j, chan);
        } else {
            printf("No channels available.\n");
            i--;
            cco_yield();
        }
    }
    char done = '1';
    cco_send(done_chan, &done);
}

void cco_main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    CCo_Channel *data_chan1 = cco_make_chan(sizeof(int), 10);
    CCo_Channel *data_chan2 = cco_make_chan(sizeof(int), 10);
    CCo_Channel *done_chan = cco_make_chan(sizeof(char), 1);

    for (int i = 0; i < 1000; i++) {
        if (i%2 == 0) {
            cco_run(worker, data_chan1, i*10);
        } else {
            cco_run(worker, data_chan2, i*10);
        }
    }
    cco_run(reciver, data_chan1, data_chan2, done_chan);

    char done;
    cco_recv(done_chan, &done);
    cco_free_chan(data_chan1);
    cco_free_chan(data_chan2);
    cco_free_chan(done_chan);
}
