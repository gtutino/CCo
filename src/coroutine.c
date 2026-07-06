#include "coroutine.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

static Ctx_Node *global_queue_head = NULL;
static pthread_mutex_t global_queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t global_queue_empty_cond = PTHREAD_COND_INITIALIZER;
// static size_t global_coroutines = 0;
static atomic_size_t total_coroutines = 0;

thread_local Ctx_Node *current_running = NULL;
static thread_local size_t local_coroutines = 0;

// Asm defined functions
void cco_save_ctx(Coroutine_Ctx *ctx);
void cco_switch_ctx(Coroutine_Ctx *ctx);
void cco_start(uint64_t rsp, void (*func)(void), void (*cco_clean)(void), uint64_t *arg);


// Special function that is called by each coroutine when it ends.
// It just clean up the allocated ctx and switch to the next coroutine.
static void cco_clean(void);


// Find the next available coroutine and update current_running to it
static void cco_set_next_current_running(void);


// A thread will go here if it don't have coroutines to run.
// Here it takes some coroutines from the global queue, so
// it can continue running.
static void cco_thread_init(void) {
    if (total_coroutines == 0) {
        exit(0);
    }

    pthread_mutex_lock(&global_queue_lock);

    while (global_queue_head == NULL) {
        pthread_cond_wait(&global_queue_empty_cond, &global_queue_lock);
    }

    // TODO: take some coroutines from the global queue

    pthread_mutex_unlock(&global_queue_lock);

    // Run the next coroutine
    cco_set_next_current_running();
    cco_switch_ctx(&current_running->ctx);
}


void cco_init(void (*cco_main)(void), int argc, char **argv, size_t threads_num) {
    // Setting it temporarely to 1 for avoinding exiting when starting the threads
    total_coroutines = 1;

    for (size_t i = 0; i < threads_num - 1; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, (void *(*)(void *))cco_thread_init, NULL);
    }

    total_coroutines = 0;

    cco_run(cco_main, 2, argc, argv);
}


void cco_run_impl(void (*func)(void), ...) {
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
    local_coroutines++;
    total_coroutines++;

    // TODO: If local_coroutines is > some threshold then move
    // some of the not running coroutines into the global queue
    // in order to give some work to do to all the threads

    // Start the coroutine
    va_list args;
    va_start(args, func);

    size_t num_args = va_arg(args, size_t);
    assert(num_args <= 6 && "The coroutines supports max 6 args for now!\n");

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


static void cco_set_next_current_running(void) {
    Ctx_Node *next_coroutine = current_running;
    while (true) {
        next_coroutine = next_coroutine->next;
        if (next_coroutine->ctx.status == NOT_RUNNING) {
            current_running = next_coroutine;
            current_running->ctx.status = RUNNING;
            return;
        }
    }
}


void cco_yield(void) {
    // Save current coroutine context
    cco_save_ctx(&current_running->ctx);

    // If we call yield when blocking for a message the
    // state shouldn't be changed
    if (current_running->ctx.status == RUNNING) {
        current_running->ctx.status = NOT_RUNNING;
    }

    cco_set_next_current_running();
    cco_switch_ctx(&current_running->ctx);
}


static void cco_clean(void) {
    // If there are no other coroutines just exit
    if (current_running->next == current_running) {
        local_coroutines--;
        total_coroutines--;
        free(current_running);
        current_running = NULL;
        cco_thread_init();
    }

    // Find the prev coroutine in the list
    Ctx_Node *prev = current_running->next;
    while (prev->next != current_running) {
        prev = prev->next;
    }

    // Remove the coroutine from the linked list
    prev->next = current_running->next;
    local_coroutines--;
    total_coroutines--;

    // Free the ctx
    Ctx_Node *to_free = current_running;
    current_running = prev;
    free(to_free);

    // Run the next one
    cco_set_next_current_running();
    cco_switch_ctx(&current_running->ctx);
}
