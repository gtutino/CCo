#ifndef CCO_LIB_
#define CCO_LIB_

// TODO: Make much of this opaque in the .c file

// For now coroutines has fixed stack size
#define STACK_BYTESIZE 4096

// This is necessary, so each coroutine knows where to look
// when it need to do context switch
static void *CCo_Ctx_location = NULL;

typedef struct {
    // Registers
    uint64_t rax;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t eflags;

    // Stack saved as is
    uint8_t stack[STACK_BYTESIZE];
} Coroutine_Ctx;


typedef struct {
    Coroutine_Ctx *coroutines_ctx;
    size_t count;
    size_t capacity;
    size_t current_running; // coroutines_ctx index
} CCo_Ctx;


// Inits the coroutine runtime and runs the first coroutine 'cco_main'.
// This should be called only ONCE.
//
// This should be used in main for calling the
// first coroutine (the real main in some sense).
// After this, the rest of the code of main
// will NOT be executed.
//
// Termination will occur when there are no coroutines left.
void cco_init(void (*cco_main)(void), ...);

// Creates a new coroutine
// When it is called it also runs that function.
//
// TODO:
//    salva gli aggiornamenti della coroutine attuale.
//    alloca nello heap lo stack di dimensione INITIAL_STACK_BYTESIZE e setta stack_size
//    inizializza i registri con i parametri passati e setta rsp,rbp in base all'indirizzo dello stack
//    setta rip in base all'indirizzo della funzione func
void cco_coroutine(void (*func)(void), ...);

// Next coroutine switch.
void yield(void);

// TODO: special function that should be called when a coroutine terminates
// so its address should be set in the stack befor rbp.
// So on termination the heap space will be cleaned and also the coroutine removed from CCo_Ctx.
// Then it switched to the next, if there is not it terminates the program.

#endif
