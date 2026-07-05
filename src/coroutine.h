#ifndef CCO_COROUTINE_H
#define CCO_COROUTINE_H

#include "../include/cco.h"
#include <stdint.h>
#include <threads.h>

// Coroutines has fixed stack size
#define COROUTINE_STACK_BYTESIZE 4096

typedef enum {
    NOT_RUNNING,
    RUNNING,
    BLOCKED,
} Coroutine_State;

typedef struct {
    // Registers (order here matters in the asm procedures)
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    Coroutine_State status;

    // Stack saved as is
    uint8_t stack[COROUTINE_STACK_BYTESIZE];
} Coroutine_Ctx;

// Circular list of coroutines contexts
typedef struct Ctx_Node Ctx_Node;
struct Ctx_Node {
    Ctx_Node *next;
    Coroutine_Ctx ctx;
};

// This is necessary, so each coroutine knows where to look when it need to do context switch
extern thread_local Ctx_Node *current_running;

#endif
