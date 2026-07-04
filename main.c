#include <stdio.h>
#include <stdlib.h>
#include "include/cco.h"

void foo(int c) {
    printf("%d\n", c);
    cco_yield();
    printf("%d\n", c + 1);
    cco_yield();
    printf("ABOBA\n");
}

void bar(int a, int b) {
    printf("Ciao\n");
    cco_yield();

    a++;
    b++;
    printf("%d %d\n", a, b);

    a++;
    b++;
    printf("%d %d\n", a, b);

    a++;
    b++;
    printf("%d %d\n", a, b);
    cco_yield();

    a++;
    b++;
    printf("%d %d\n", a, b);
    printf("FINE\n");
}


void cco_main(void) {
    cco_run(foo, 1, 1);
    cco_run(bar, 2, 2, 3);
    cco_yield();
    printf("ciao cco_main\n");
}

int main(void) {
    cco_run(cco_main, 0, 0);
}
