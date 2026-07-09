// Stackful Coroutine implementation in C.
// This is the public interface of the library.
//
// [NOTE]: by default the thread pool will have thread number = number of cores.
// If you want to set a specific number of threads just define CCO_THREAD_NUM
// in the building script by passing -DCCO_THREAD_NUM=$(NUM) option to your compiler.

#ifndef CCO_LIB_H
#define CCO_LIB_H

#include <stddef.h>


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
#define cco_run(func, ...) cco_run_impl((void(*)(void))func, ##__VA_ARGS__, &CCO_ARGS_END_SENTINEL)
static const char CCO_ARGS_END_SENTINEL;


// Channels can be used for communication between the coroutines.
// The communication is synchronous and unbuffered,
// so send and recv are blocking (and can cause context switch).
typedef struct CCo_Channel CCo_Channel;


// Create a new channel.
CCo_Channel *cco_make_chan(size_t payload_size);


// Create a new buffered channel.
// CCo_Channel *cco_make_bufchan(size_t payload_size, size_t buffer_count);


// Free the channel.
//
// [NOTE]: sending and reciving from a freed channel causes UB.
void cco_free_chan(CCo_Channel *chan);


// Send data through the channel (will be sent payload_size byte of data).
void cco_send(CCo_Channel *chan, void *data);


// Recive data through the channel.
void cco_recv(CCo_Channel *chan, void *dest);


// Select [TODO: better description].
//
// Example:
// cco_select(
//     cco_send, chan, &data1,
//     cco_recv, chan, &dest1,
//     cco_send, chan, &data2,
//     ...
//     ..
//  );
// size_t cco_select_impl(char arg_start, ...);
// #define cco_select(arg_start, ...) cco_run_impl(arg_start, ##__VA_ARGS__, NULL)


// Context switch to the next coroutine.
void cco_yield(void);


#endif
