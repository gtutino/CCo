#include <stdio.h>
#include <stdlib.h>
#include "include/cco.h"

void foo(int c) {
    printf("%d\n", c);
}

void func(int a, int b, char *str) {
    printf("%d %d %s\n", a, b, str);
    cco_run((void(*)(void))foo, 1, 10);
    printf("%d %d %s\n", a, b, str);
}

int main(void) {
    cco_run((void(*)(void))func, 3, 0, 1, "Hello World!");

    return 0;
}
