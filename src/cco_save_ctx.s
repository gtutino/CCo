.global cco_save_ctx
cco_save_ctx:
        // rdi contains the pointer to 'current_running->ctx'
    movq %rsp, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq %rbx, 24(%rdi)
    movq %r12, 32(%rdi)
    movq %r13, 40(%rdi)
    movq %r14, 48(%rdi)
    movq %r15, 56(%rdi)

    // Get the rip of cc_run_impl by just looking before the base pointer
    movq 8(%rbp), %rax
    movq %rax, 16(%rdi)

    ret
