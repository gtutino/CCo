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
static size_t global_coroutines = 0;
static atomic_size_t total_coroutines = 0;

thread_local Ctx_Node *current_running = NULL;
static thread_local size_t local_coroutines = 0;

// Saving the coroutine "real" initial stack here, because when returning
// to 'cco_thread_init' from the last coroutine we need a stack.
static thread_local uint64_t stack_rsp;
static thread_local uint64_t stack_rbp;

static size_t threads_num;

// Asm defined functions
void cco_save_ctx(Coroutine_Ctx *ctx);
void cco_switch_ctx(Coroutine_Ctx *ctx);
void cco_free_n_switch_ctx(void *to_free, Coroutine_Ctx *ctx);
void cco_start(uint64_t rsp, void (*func)(void), void (*cco_clean)(void), uint64_t *arg);
void cco_save_stack_pointers(uint64_t *stack_rsp, uint64_t *stack_rbp);
void cco_goto_init(uint64_t stack_rsp, uint64_t stack_rbp, void (*cco_thread_init)(void));


// Special function that is called by each coroutine when it ends.
// It just clean up the allocated ctx and switch to the next coroutine.
static void cco_clean(void);


// Find the next available coroutine and update current_running to it
static void cco_set_next_current_running(void);


// A thread will go here if it don't have coroutines to run.
// Here it takes some coroutines from the global queue, so
// it can continue running.
static void cco_global_pool_thread(void) {
    pthread_mutex_lock(&global_queue_lock);

    while (global_queue_head == NULL) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 3;
        pthread_cond_timedwait(&global_queue_empty_cond, &global_queue_lock, &ts);
    }

    // Get some global coroutines
    // size_t number_to_get = 1 + global_coroutines / (threads_num - 1);
    // TODO
    size_t number_to_get = global_coroutines;

    Ctx_Node *first = global_queue_head;
    Ctx_Node *last = first;
    for (size_t i = 0; i < number_to_get - 1; i++) {
        last = last->next;
    }
    global_queue_head = last->next;
    last->next = first;
    current_running = last;

    global_coroutines -= number_to_get;
    local_coroutines += number_to_get;

    pthread_mutex_unlock(&global_queue_lock);

    // Run the next coroutine
    cco_set_next_current_running();
    cco_switch_ctx(&current_running->ctx);
}

// Called by 'cco_clean'.
// Check exit condition and frees the last coroutine.
static void cco_clean_thread(void) {
    // We returned from the last coroutine so we need to free it
    if (current_running != NULL) {
        free(current_running);
        current_running = NULL;
    }

    if (total_coroutines == 0) {
        exit(0);
    }

    cco_global_pool_thread();
}


static void cco_init_thread(void) {
    cco_save_stack_pointers(&stack_rsp, &stack_rbp);
    cco_global_pool_thread();
}


void cco_init(void (*cco_main)(void), int argc, char **argv, size_t t_n) {
    threads_num = t_n;

    for (size_t i = 0; i < threads_num - 1; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, (void *(*)(void *))cco_init_thread, NULL);
    }

    cco_save_stack_pointers(&stack_rsp, &stack_rbp);
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
        coroutine->next = current_running_next;                // TODO: sometimes segfaults here
    }
    current_running = coroutine;
    coroutine->ctx.status = RUNNING;
    local_coroutines++;
    total_coroutines++;

    // If we have more coroutines w.r.t the mean amout for each
    // thread, we move the excess to the global queue.
    // size_t ratio = (total_coroutines / threads_num);
    // TODO
    //    if (local_coroutines > ratio + 1) {
    if (local_coroutines > 1 && threads_num > 1) {
        // size_t number_to_take = local_coroutines - 1 - ratio;
        size_t number_to_take = 1;

        // Remove number_to_take coroutines from the local queue.
        //
        // Taking them as a slice of local queue:
        // current_running -> first -> ... -> last -> ...
        //                    |__________________|
        //
        Ctx_Node *first = current_running->next;
        Ctx_Node *last = first;
        for (size_t i = 0; i < number_to_take - 1; i++) {
            last = last->next;
        }
        current_running->next = last->next;

        // Adding to global queue
        pthread_mutex_lock(&global_queue_lock);

        if (global_queue_head == NULL) {
            global_queue_head = first;
            last->next = NULL;
        } else {
            last->next = global_queue_head;
            global_queue_head = first;
        }
        global_coroutines += number_to_take;
        local_coroutines -= number_to_take;

        pthread_cond_broadcast(&global_queue_empty_cond);
        pthread_mutex_unlock(&global_queue_lock);
    }

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

    cco_start(stack_top, func, cco_clean, arg);
}


static void cco_set_next_current_running(void) {

    Ctx_Node *next_coroutine;

set_next:
    // Look for the next coroutine
    next_coroutine = current_running->next;
    for (size_t i = 0; i < local_coroutines; i++) {
        if (next_coroutine->ctx.status == NOT_RUNNING) {
            current_running = next_coroutine;
            current_running->ctx.status = RUNNING;
            return;
        }
        next_coroutine = next_coroutine->next;
    }

    // All coroutines are BLOCKED, trying to get something from global queue.
    // TODO: maybe don't take all but some fixed num of coroutines.
    // TODO: here there is busy wait, it can be fixed if channels send a special
    // signal after they unblock a coroutine.
    pthread_mutex_lock(&global_queue_lock);

    if (global_queue_head == NULL) {
        pthread_mutex_unlock(&global_queue_lock);
        goto set_next;
    } else {
        // current_running -> [global queue] -> current_running_next
        Ctx_Node *current_runnning_next = current_running->next;
        current_running->next = global_queue_head;

        Ctx_Node *node = global_queue_head;
        for (size_t i = 0; i < global_coroutines - 1; i++) {
            node = node->next;
        }

        node->next = current_runnning_next;
        global_queue_head = NULL;

        local_coroutines += global_coroutines;
        global_coroutines = 0;

        pthread_mutex_unlock(&global_queue_lock);
        goto set_next;
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
    local_coroutines--;
    total_coroutines--;

    // If there are no other coroutines just go to init function.
    // We use 'cco_goto_init' for restoring the initial stack and go to init.
    if (current_running->next == current_running) {
        cco_goto_init(stack_rsp, stack_rbp, cco_clean_thread);
    }

    // Remove the coroutine from the linked list
    Ctx_Node *prev = current_running->next;
    while (prev->next != current_running) {
        prev = prev->next;
    }
    prev->next = current_running->next;
    Ctx_Node *to_free = current_running;
    current_running = prev;

    // Run the next one and free 'to_free'.
    cco_set_next_current_running();
    cco_free_n_switch_ctx(to_free, &current_running->ctx);
}
