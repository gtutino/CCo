// Stackful Coroutine implementation in C.
// This is the public interface of the library.
//
// [NOTE]:
// By default the thread pool will have thread number = number of cores.
// If you want to set a specific number of threads just define CCO_THREADS env variable.

#ifndef CCO_H_
#define CCO_H_

#include <stddef.h>


// This function is NOT defined in the library.
// This MUST be defined by the user and it is
// the new entry point.
void cco_main(int argc, char **argv);


// Creates a new coroutine.
//
// [NOTE]:
// The variadic args takes the args for 'func' (max 6 allowed).
// ALL the variadic args MUST HAVE size <= 8 byte, so you should pass large structs only by pointer.
//
// Also, passing a pointer that points into the previous coroutine stack can be dangerous.
// If the previous coroutine ends before the current one you'll have use after free.
void cco_run_impl(void (*func)(void), ...);
#define cco_run(func, ...) cco_run_impl((void(*)(void))func, ##__VA_ARGS__, &CCO_ARGS_END_SENTINEL)
static const char CCO_ARGS_END_SENTINEL;


// Channels can be used for communication between the coroutines.
typedef struct CCo_Channel CCo_Channel;


// Creates a new channel.
//
// [NOTE]:
// 'buffer_capacity' is the max number of messages that can be hold in the buffer.
// If 'buffer_capacity' = 0 then the channel will be unbuffered.
// If there isn't a buffer or the buffer is full send and recv can be blocking.
CCo_Channel *cco_make_chan(size_t payload_size, size_t buffer_capacity);


// Free the channel.
//
// [NOTE]: sending and reciving from a freed channel causes UB.
void cco_free_chan(CCo_Channel *chan);


// Send data through the channel (will be sent payload_size byte of data).
void cco_send(CCo_Channel *chan, void *data);


// Recive data through the channel.
void cco_recv(CCo_Channel *chan, void *dest);


// The select function takes multiple channels action (send/recv)
// and randomly choose one of the available channels, doing the action.
// The return number corresponds to the number of used channel
// (starting from 1), 0 means that no channel was available.
//
// Example:
// cco_select(
//     cco_send, chan, &data1,
//     cco_recv, chan, &dest1,
//     cco_send, chan, &data2,
//     ...
//     ..
//  );
//
// [NOTE]
// Right now the max amout of actions is 16.
typedef void (*cco_chan_func)(CCo_Channel *chan, void *data);
size_t cco_select_impl(cco_chan_func first_fun, ...);
#define cco_select(first_fun, ...) cco_select_impl(first_fun, __VA_ARGS__, NULL)


// Context switch to the next coroutine.
void cco_yield(void);


#endif
