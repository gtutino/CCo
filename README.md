# CCo - C Coroutines

## NOTE
This is still a work in progress!

## Overview
This library implements coroutines with unbufferend (synchronous) message-passing in C.\
The coroutines can run in a single thread and also in multiple threads using a thread pool of fixed system threads.

The API (`include/cco.h`) has functions that are simiar to the concurrent primitives of Go,\
so if you already know Go programming language then the functions in the API will be familiar to you.

This project was made as an assignment for the Languages for Concurrency and Distribution course of the University of Padua.\
It is written in C (C11) and has no external dependencies.

## Design
[TODO]

## Limitations
[TODO]

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
