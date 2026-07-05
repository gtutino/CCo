#include <stdio.h>
#include <stdlib.h>
#include "include/cco.h"

void worker(CCo_Channel *chan, int x) {
    for (int i = x; i < x + 10; i++) {
        cco_send(chan, &i);
    }
}

void cco_main(void) {
    CCo_Channel *chan = cco_make_chan(sizeof(int));
    cco_run(worker, 2, chan, 3);
    cco_run(worker, 2, chan, 10);
    cco_run(worker, 2, chan, 27);
    cco_run(worker, 2, chan, 43);
    for (int i = 0; i < 40; i++) {
        int j;
        cco_recv(chan, &j);
        printf("Recived %d\n", j);
    }
}

int main(void) {
    cco_init(cco_main, 0, NULL, 4);
}
