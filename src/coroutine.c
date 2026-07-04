#include "coroutine.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

// Asm defined functions
void cco_save_ctx(Coroutine_Ctx *ctx);
void cco_yield_run_next(Coroutine_Ctx *ctx);
void cco_start(uint64_t rsp, void (*func)(void), void (*cco_clean)(void), uint64_t *arg);

// Special function that is called by each coroutine when it ends.
// It just clean up the allocated ctx and switch to the next coroutine.
static void cco_clean(void);

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

    cco_start(rsp, func, cco_clean, arg);
}


static void cco_yield_swap(void) {
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


void cco_yield(void) {
    // Save current coroutine context
    cco_save_ctx(&current_running->ctx);
    current_running->ctx.status = NOT_RUNNING;

    cco_yield_swap();
}


static void cco_clean(void) {
    // If there are no other coroutines just exit
    if (current_running->next == current_running) {
        free(current_running);
        exit(0);
    }

    // Find the prev coroutine in the list
    Ctx_Node *prev = current_running->next;
    while (prev->next != current_running) {
        prev = prev->next;
    }

    // Remove the coroutine from the linked list
    prev->next = current_running->next;

    // Free the ctx
    Ctx_Node *to_free = current_running;
    current_running = prev;
    free(to_free);
    cco_yield_swap();
}
