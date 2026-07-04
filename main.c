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

void func(int a, int b) {
    cco_run(foo, 1, 1);

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
}

int main(void) {
    cco_run(func, 3, 2, 3);

    return 0;
}
