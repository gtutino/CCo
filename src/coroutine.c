#include "coroutine.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

#define GLOBAL_QUEUE_WAIT_TIMEOUT 3

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
void cco_start(uint64_t rsp, void (*func)(void), void (*cco_clean)(void), uint64_t *arg);
void cco_save_stack_pointers(uint64_t *stack_rsp, uint64_t *stack_rbp);
void cco_free_n_switch_ctx(void *to_free, Coroutine_Ctx *ctx, uint64_t stack_rsp, uint64_t stack_rbp);
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
        ts.tv_sec += GLOBAL_QUEUE_WAIT_TIMEOUT;
        pthread_cond_timedwait(&global_queue_empty_cond, &global_queue_lock, &ts);
    }

    // Get some global coroutines
    size_t number_to_get = (global_coroutines + threads_num - 2) / (threads_num - 1);

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


int main(int argc, char **argv) {
    threads_num = 0;
    char *env_threads = getenv("CCO_THREADS");

    if (env_threads != NULL) {
        int threads = atoi(env_threads);
        if (threads <= 0) {
            fprintf(stderr, "[ERROR]: CCO_THREADS must be > 0\n");
            exit(1);
        }
        threads_num = threads;
    }

    // Thread number set to cores number if CCO_THREADS env is not set
    if (threads_num == 0) {
        long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
        if (num_cores < 1) {
            fprintf(stderr, "[WARNING]: Failed to detect cores number, running in single thread.\n");
            threads_num = 1;
        } else {
            threads_num = num_cores;
        }
    }

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
        coroutine->next = current_running_next;
    }
    current_running = coroutine;
    coroutine->ctx.status = RUNNING;
    local_coroutines++;
    total_coroutines++;

    // If we have more coroutines w.r.t the mean amout for each
    // thread, we move the excess to the global queue.
    if (threads_num > 1) {
        size_t ratio = (total_coroutines + threads_num - 1) / threads_num;

        if (local_coroutines > ratio) {
            size_t number_to_take = local_coroutines - ratio;

            // We need to save at least 2 coroutines locally.
            // This is very important, in that case we save the old current_running
            // that is the current stack (giving that causes UB) and the current_running.
            size_t max_takeable = local_coroutines - 2;
            if (number_to_take > max_takeable) {
                number_to_take = max_takeable;
            }

            if (number_to_take > 0) {
                // Remove number_to_take coroutines from the local queue.
                //
                // Taking them as a slice of local queue
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
        }
    }

    // Start the coroutine
    va_list args;
    va_start(args, func);

    uint64_t arg[7];
    arg[0] = va_arg(args, uint64_t);

    size_t i = 1;
    while (((void *)arg[i-1]) != &CCO_ARGS_END_SENTINEL && i < 7) {
        arg[i] = va_arg(args, uint64_t);
        i++;
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
    // TODO: here there is busy wait, it can be fixed if channels send a special
    // signal after they unblock a coroutine.
    pthread_mutex_lock(&global_queue_lock);

    if (global_queue_head == NULL) {
        pthread_mutex_unlock(&global_queue_lock);
        goto set_next;
    } else {
        // Get some global coroutines
        size_t number_to_get = (global_coroutines + threads_num - 2) / (threads_num - 1);
        Ctx_Node *current_runnning_next = current_running->next;
        current_running->next = global_queue_head;

        Ctx_Node *first = global_queue_head;
        Ctx_Node *last = first;
        for (size_t i = 0; i < number_to_get - 1; i++) {
            last = last->next;
        }
        global_queue_head = last->next;
        last->next = current_runnning_next;

        global_coroutines -= number_to_get;
        local_coroutines += number_to_get;

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
    // We don't free it here because we are on its stack
    // and that will cause an UB, because the call after the
    // free will write the rip to the stack).
    cco_set_next_current_running();
    cco_free_n_switch_ctx(to_free, &current_running->ctx, stack_rsp, stack_rbp);
}
