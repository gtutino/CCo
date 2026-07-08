#include <stdio.h>
#include <stdlib.h>

#define CCO_THREAD_NUM 4
#include "include/cco.h"

void worker(CCo_Channel *chan, int x) {
    for (int i = x; i < x + 10; i++) {
        cco_send(chan, &i);
    }
}

void reciver(CCo_Channel *data_chan, CCo_Channel *done_chan) {
    for (int i = 0; i < 10000; i++) {
        int j;
        cco_recv(data_chan, &j);
        printf("Recived %d\n", j);
    }
    char done = '1';
    cco_send(done_chan, &done);
}

void cco_main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    CCo_Channel *data_chan = cco_make_chan(sizeof(int));
    CCo_Channel *done_chan = cco_make_chan(sizeof(char));

    for (int i = 0; i < 1000; i++) {
        cco_run(worker, 2, data_chan, i*10);
    }
    cco_run(reciver, 2, data_chan, done_chan);

    char done;
    cco_recv(done_chan, &done);
    cco_free_chan(data_chan);
    cco_free_chan(done_chan);
}
