.global cco_yield_run_next
// Restore the registers to switch corutine
cco_yield_run_next:
    movq 0(%rdi), %rsp
    movq 8(%rdi), %rbp
    movq 24(%rdi), %rbx
    movq 32(%rdi), %r12
    movq 36(%rdi), %r13
    movq 48(%rdi), %r14
    movq 56(%rdi), %r15
    jmp  *16(%rdi)
