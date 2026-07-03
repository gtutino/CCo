#ifndef CCO_LIB_
#define CCO_LIB_

#include "../src/cco_private_api.h"

// Stackful Coroutine implementation in C.
// This is the public interface of the library.

// Creates a new coroutine.
//
// [NOTE]
// Your 'main' function should be used
// just for running a real 'main' in the
// form of coroutine.
// After calling for the first time this in main
// the rest of the main function will NOT be executed.
//
// See examples for more details.
#define cco_run(func, num_args, ...) do {                   \
    __cco_save_ctx();                                       \
    __cco_run((void(*)(void))func, num_args, __VA_ARGS__);  \
} while (0);                                                \


// Switch to next coroutine.
void cco_yield(void);

#endif
