#include <stdio.h>
#include <stdlib.h>
#include "include/cco.h"

void worker(CCo_Channel *chan, int x) {
    for (int i = x; i < x + 10; i++) {
        cco_send(chan, &i);
    }
}

void reciver(CCo_Channel *chan) {
    for (int i = 0; i < 100000; i++) {
        int j;
        cco_recv(chan, &j);
        printf("Recived %d\n", j);
    }
}

void cco_main(void) {
    CCo_Channel *chan = cco_make_chan(sizeof(int));
    for (int i = 0; i < 10000; i++) {
        cco_run(worker, 2, chan, i*10);
    }

    cco_run(reciver, 1, chan);
}

int main(void) {
    cco_init(cco_main, 0, NULL, 4);
}
