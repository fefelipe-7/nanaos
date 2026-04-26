.section .text
.global context_switch
/*
 * context_switch(old_sp_ptr, new_sp)
 *   rdi = address where current rsp should be saved (uint64_t **)
 *   rsi = new stack pointer to load (uint64_t *)
 *
 * Saves caller registers on current stack, stores rsp into *old_sp_ptr,
 * then loads new_sp into rsp and restores registers, returning to the
 * new context via `ret`.
 */
context_switch:
    /* save general-purpose registers */
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax

    /* store current rsp into *old_sp_ptr */
    movq %rsp, (%rdi)

    /* switch to new stack */
    movq %rsi, %rsp

    /* restore registers (note order is reverse of saved) */
    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rbp
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15

    ret

    .section .note.GNU-stack,"",@progbits
