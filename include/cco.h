#ifndef CCO_LIB_H
#define CCO_LIB_H

#include <stddef.h>

// Stackful Coroutine implementation in C.
// This is the public interface of the library.


// Starts the main coroutine and inits the runtime.
// The standard main function MUST be used to call this function.
//
// In this way:
//  int main(int argc, char **argv) {
//      cco_init(cco_main, argc, argv, 4);
//  }
//
// 'threads_num' is the num of threads that will be in the thread pool.
void cco_init(void (*cco_main)(void), int argc, char **argv, size_t threads_num);


// Creates a new coroutine.
// This must be called by another coroutine.
//
// [NOTE]: The variadic args takes the args for 'func' (max 6 allowed).
// The first variadic arg must be the number of args.
void cco_run_impl(void (*func)(void), ...);
#define cco_run(func, ...) cco_run_impl((void(*)(void))func, __VA_ARGS__);


// Channels can be used for communication between the coroutines.
// The communication is synchronous and unbuffered,
// so send and recv are blocking (and can cause context switch).
typedef struct CCo_Channel CCo_Channel;


// Create a new channel.
CCo_Channel *cco_make_chan(size_t payload_size);


// Free the channel.
//
// [NOTE]: sending and reciving from a freed channel causes UB.
void cco_free_chan(CCo_Channel *chan);


// Send data through the channel (will be sent payload_size byte of data).
void cco_send(CCo_Channel *chan, void *data);


// Recive data through the channel.
void cco_recv(CCo_Channel *chan, void *dest);


// Context switch to the next coroutine.
void cco_yield(void);


#endif
