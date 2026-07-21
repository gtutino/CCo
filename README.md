# CCo - C Coroutines

## Overview
CCo implemetes Go-style coroutines in C.\
Coroutines can use buffered and unbuffered channels to communicate in a message-passing style.
The coroutines can run in a single thread or in multiple threads using a thread pool of fixed os threads.

The API (`include/cco.h`) has all the function to handle coroutines and channels,\
if you already know Go programming language then the functions in the API will be familiar to you.

This project was made as an assignment for the Languages for Concurrency and Distribution course of the University of Padua.\
It is written in C (C11) and has no external dependencies.

*[NOTE]* This is just a fun personal project, so I don't recommend using it in industrial projects.

## Architecture

Each coroutine has a context that saves the register state, the executing stack and infos about its running state.
They are organied in a circular queue (using a doubly linked list) and the "scheduling" is just round robin,\
so when a coroutine yields the current thread just looks at the next node of the list and do the context switch.\
The context switch it's implemented in asm with custom procedures.

Coroutine free is handled automatically by the coroutine itself. When a coroutine starts the address of a cleanup function is put at the top of the stack, so when the coroutine exit the last stack frame it will "return" there and it will cleanup itself (and also then switch to next corutine).

### Thread Pool

Coroutines can run with multiple threads in a thread pool way, by default thread pool has N threads where N is the number of cpu
cores. By the way, CCO_THREADS enviroment var can be set to a fixed amount of thread by the user.

Each thread has its own local coroutines organized as said before (circular queue) and cannot access to other threads queues.\
So there is no thread syncronization when accessing local coroutines.\
Then there is a global queue that can be used to pull and push coroutines by all the threads, so its access is handles by a mutex.

A thread will push to the global queue if its local coroutines are more of a fixed threshold (based on the total number of coroutines) and pull in the opposite situation, in this way all the threads will have the same amount of coroutines.\
This behaviour happens every time there is a context switch, since there is no preemption if the coroutines don't switch frequently there can be some misalignments.

### Channels

Channels can be used by multiple coroutines without limits, so they are general MxN channels.\
They can be buffered (with a fixed size) or unbuffered, so the interaction can also be async.\
The order of messages is *always* preserved.
A channel structure is basicall composed of two queues (send and recv queue) and the message data is just copied to destination through memcpy.

## Limitations
- Coroutines can take up to 6 args of maximum 8 bytes each.
- Select function can take up to 16 channel actions.
- The stack of each coroutine is fixed to a constant value.
- The code in `src` doesn't compile with compiler optimization and needs `-fno-omit-frame-pointer` flag for ensuring correct behavior.
- There is no preemption, so if a coroutine doesn't use yield or channels other coroutines starve.

## Requirements
- x86_64 architecture and OS with System V AMD64 calling convention (I tested on a linux distro).
- C11-compatible compiler.
- Make (optional, you can just put all *.c and *.s files into the compiler).

## How to build
Just run
``` bash
$ make
```

And you'll find an example `main` executable.

## References
- Libmill, a similar project (single-threaded):\
    https://github.com/sustrik/libmill
- LCD course:\
    https://unipd.coursecatalogue.cineca.it/corsi/2025/2634/insegnamenti/2025/52879_491001_69754/2025/52879
