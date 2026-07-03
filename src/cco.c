#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <threads.h>
#include "../include/cco.h"

// Coroutines has fixed stack size
#define COROUTINE_STACK_BYTESIZE 4096

typedef enum {
    NOT_RUNNING,
    RUNNING,
    BLOCKED,
} Coroutine_State;

typedef struct {
    // Registers
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

// Circular linked list of coroutines contexts
typedef struct Ctx_Node Ctx_Node;
struct Ctx_Node {
    Ctx_Node *next;
    Coroutine_Ctx ctx;
};

// This is necessary, so each coroutine knows where to look
// when it need to do context switch
static thread_local Ctx_Node *current_running = NULL;

void cco_save_ctx(Coroutine_Ctx *ctx);
void cco_yield_run_next(Coroutine_Ctx *ctx);

void *cco_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR]: OOM error when allocating %zu bytes, aborting.\n", size);
        exit(1);
    }
    return ptr;
}

// TODO: special function that should be called when a coroutine terminates
// so its address should be set in the stack befor rbp.
// So on termination the heap space will be cleaned and also the coroutine removed from CCo_Ctx.
// Then it switched to the next, if there is not it terminates the program.
void cco_clean(void) {
    printf("COROUTINE FINISH\n");
    exit(0);
}

void cco_run_impl(void (*func)(void), size_t num_args, ...) {
    // Save current coroutine context
    if (current_running != NULL) {
        cco_save_ctx(&current_running->ctx);
        current_running->ctx.status = NOT_RUNNING;
    }

    // Allocate the new coroutine node and link in the linked list
    Ctx_Node *coroutine = cco_malloc(sizeof(Ctx_Node));
    if (current_running == NULL) {
        coroutine->next = coroutine;
    } else {
        Ctx_Node *current_running_next = current_running->next;
        current_running->next = coroutine;
        coroutine->next = current_running_next;
    }
    current_running = coroutine;
    coroutine->ctx.status = RUNNING;

    // Start the coroutine
    assert(num_args <= 6 && "The coroutines supports max 6 args for now!\n");

    va_list args;
    va_start(args, num_args);
    uint64_t arg[6];
    for(size_t i = 0; i < num_args; i++) {
        arg[i] = va_arg(args, uint64_t);
    }
    va_end(args);

    uint64_t stack_top = (uint64_t)(coroutine->ctx.stack) + COROUTINE_STACK_BYTESIZE;
    stack_top = stack_top & -16LL; // 16-byte alignment required by ABI
    uint64_t rsp = (uint64_t)(stack_top - 8);

    // AT&T syntax: instr source, dest
    __asm__ __volatile__ (
        // New stack
        "movq %0, %%rsp\n\t"
        "movq %0, %%rbp\n\t"

        // Set rip to cco_clean
        "pushq %3\n\t"

        // Arguments
        "movq 0(%2), %%rdi\n\t"
        "movq 8(%2), %%rsi\n\t"
        "movq 16(%2), %%rdx\n\t"
        "movq 24(%2), %%rcx\n\t"
        "movq 32(%2), %%r8\n\t"
        "movq 40(%2), %%r9\n\t"

        // "Call" the function
        "jmp *%1\n\t"

        :                                                    // Dest
        : "r" (rsp), "r" (func), "r" (arg), "r" (cco_clean)  // Source
        : "rdi", "rsi", "rdx", "rcx", "r8", "r9"             // Clobber list
    );
}

void cco_yield(void) {
    // Save current coroutine context
    if (current_running != NULL) {
        cco_save_ctx(&current_running->ctx);
        current_running->ctx.status = NOT_RUNNING;
    }

    // Find the next coroutine
    bool found = false;
    Ctx_Node *next_coroutine = current_running;
    while (!found) {
        next_coroutine = next_coroutine->next;
        if (next_coroutine->ctx.status == NOT_RUNNING) {
            current_running = next_coroutine;
            current_running->ctx.status = RUNNING;
            found = true;
        }
    }

    // Run it
    cco_yield_run_next(&current_running->ctx);
}
