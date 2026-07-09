.global cco_save_ctx
.global cco_switch_ctx
.global cco_free_n_switch_ctx
.global cco_start
.global cco_save_stack_pointers
.global cco_goto_init

// Save the registers to the coroutine ctx.
// First arg (rdi) contains the pointer to 'current_running->ctx'.
//
// Current state of the stack when called by cc_run_impl:
// ============================================
//  [0] RIP for return to cc_run_impl
//  [1] RBP of callee of cc_run_impl              <--- (%rbp)
//  [2] RIP for return to callee of cc_run_impl
//  [3] Previous stack frame
// ============================================
cco_save_ctx:
    // rbp points to the old rbp value of the callee of cc_run_impl [1]
    movq (%rbp), %rax
    movq %rax, 8(%rdi)

    // rsp points before the rip of callee of cc_run_impl [3]
    leaq 16(%rbp), %rax
    movq %rax, 0(%rdi)

    // rip of the callee of cc_run_impl [2]
    movq 8(%rbp), %rax
    movq %rax, 16(%rdi)

    // Standard registers
    movq %rbx, 24(%rdi)
    movq %r12, 32(%rdi)
    movq %r13, 40(%rdi)
    movq %r14, 48(%rdi)
    movq %r15, 56(%rdi)
    ret


// Restore the registers from courutine ctx and frees the previous one.
// First arg (rdi) contains the pointer to 'current_running->ctx'.
cco_switch_ctx:
    movq 0(%rdi), %rsp
    movq 8(%rdi), %rbp
    movq 24(%rdi), %rbx
    movq 32(%rdi), %r12
    movq 40(%rdi), %r13
    movq 48(%rdi), %r14
    movq 56(%rdi), %r15
    jmp  *16(%rdi)


// Frees a coroutine and then do the context switch.
// First arg (rdi) contains the pointer to a coroutine to free.
// Second arg (rsi) contains the pointer to 'current_running->ctx'.
// Third arg (rdx) contains stack_rsp.
// Fourth arg (rcx) contains stack_rbp.
cco_free_n_switch_ctx:
    // Restoring initial stack
    movq %rdx, %rsp
    movq %rcx, %rbp

    movq %rsi, %r12     /* Saving rsi because free can alter it */
    call free
    movq %r12, %rdi
    jmp cco_switch_ctx


// Starts the coroutine.
// First arg  (rdi) contains the stack pointer
// Second arg (rsi) contains the coroutine func pointer
// Third arg  (rdx) contains the coroutine return function (clean func)
// Fourth arg (rcx) contains the pointer to coroutine args array (6)
cco_start:
    // Change stack
    movq %rdi, %rsp
    movq %rdi, %rbp

    // Set return address to the clean function
    pushq %rdx

    // Saving the function pointer before setting the args
    movq %rsi, %rax

    // Pass args
    movq 0(%rcx), %rdi
    movq 8(%rcx), %rsi
    movq 16(%rcx), %rdx
    movq 32(%rcx), %r8
    movq 40(%rcx), %r9
    movq 24(%rcx), %rcx

    // Jump to function code
    jmp *%rax


// Saves rsp and rbp
cco_save_stack_pointers:
    movq %rsp, (%rdi)
    movq %rbp, (%rsi)
    ret


// Restore rsp, rbp and jump to the init
cco_goto_init:
    movq %rdi, %rsp
    movq %rsi, %rbp
    jmp *%rdx
