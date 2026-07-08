// Stackful Coroutine implementation in C.
// This is the public interface of the library.

#ifndef CCO_LIB_H
#define CCO_LIB_H

#include <stddef.h>

// By default it's single threaded.
#ifndef CCO_THREAD_NUM
#define CCO_THREAD_NUM 1
#endif


// This function is NOT defined in the library.
// This MUST be defined by the user and it is
// the new entry point.
void cco_main(int argc, char **argv);


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
