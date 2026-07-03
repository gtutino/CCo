#ifndef CCO_LIB_PRIVATE_
#define CCO_LIB_PRIVATE_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <threads.h>

// Coroutines has fixed stack size
#define COROUTINE_STACK_BYTESIZE 4096

typedef enum {
    NOT_RUNNING,
    RUNNING,
    BLOCKED,
} Coroutine_State;

typedef struct {
    Coroutine_State state;

    // Registers
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip;

    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    // Stack saved as is
    uint8_t stack[COROUTINE_STACK_BYTESIZE];
} Coroutine_Ctx;

// Circular linked list of coroutines contexts
typedef struct Ctx_Node Ctx_Node;
struct Ctx_Node {
    Ctx_Node *next;
    Coroutine_Ctx ctx;
};

// This is necessary, so each coroutine knows where to look
// when it need to do context switch
extern thread_local Ctx_Node *current_running;

void __cco_run(void (*func)(void), size_t num_args, ...);

// TODO: save the state of the current coroutine
// Maybe use a macro that do this job and then calls the "real function".
// So in the macro we can easily save the rsp, rbp BEFORE the call and the rip should be just incremented by 16 bytes.
#define __cco_save_ctx() do {        \
    if (current_running != NULL) {   \
        printf("CTX SAVE\n");        \
    }                                \
} while (0);                         \

#endif
