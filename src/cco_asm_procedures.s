.global cco_save_ctx
.global cco_switch_ctx
.global cco_start

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


// Restore the registers from courutine ctx.
// First arg (rdi) contains the pointer to 'current_running->ctx'.
cco_switch_ctx:
    movq 0(%rdi), %rsp
    movq 8(%rdi), %rbp
    movq 24(%rdi), %rbx
    movq 32(%rdi), %r12
    movq 36(%rdi), %r13
    movq 48(%rdi), %r14
    movq 56(%rdi), %r15
    jmp  *16(%rdi)


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
