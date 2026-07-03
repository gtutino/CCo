#ifndef CCO_LIB_
#define CCO_LIB_

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
void cco_run(void (*func)(void), size_t num_args, ...);

// Switch to next coroutine.
void cco_yield(void);

#endif
