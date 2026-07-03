#ifndef CCO_LIB_
#define CCO_LIB_

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
void cco_run_impl(void (*func)(void), size_t num_args, ...);
#define cco_run(func, num_args, ...) cco_run_impl((void(*)(void))func, num_args, __VA_ARGS__);



// Switch to next coroutine.
void cco_yield(void);

#endif
