/*
 * interrupt_stubs.S
 *
 * Each stub pushes a dummy error code (0) for exceptions that don't push one,
 * then pushes the vector number, and jumps to isr_common.
 *
 * Stack layout on entry to isr_common (low → high address):
 *   [RSP+ 0]  vector
 *   [RSP+ 8]  error_code  (0 or CPU-pushed)
 *   [RSP+16]  RIP   \
 *   [RSP+24]  CS     |  pushed by CPU on exception
 *   [RSP+32]  RFLAGS |
 *   [RSP+40]  RSP   /
 *   [RSP+48]  SS
 */

.section .text

/* ── Macro: exception WITHOUT CPU error code ─────────────────────────── */
.macro ISR_NOERR num
.global isr\num
isr\num:
    pushq $0          /* dummy error code */
    pushq $\num       /* vector number    */
    jmp   isr_common
.endm

/* ── Macro: exception WITH CPU error code ────────────────────────────── */
.macro ISR_ERR num
.global isr\num
isr\num:
    pushq $\num       /* vector number (error code already on stack) */
    jmp   isr_common
.endm

/* ── Exception stubs ─────────────────────────────────────────────────── */
ISR_NOERR  0    /* #DE  Divide-by-Zero      */
ISR_NOERR  6    /* #UD  Invalid Opcode      */
ISR_ERR    8    /* #DF  Double Fault        */
ISR_ERR   13    /* #GP  General Protection  */
ISR_ERR   14    /* #PF  Page Fault          */

/* ── IRQ stubs (vectors 32..47) ─────────────────────────────────────── */
ISR_NOERR  32   /* IRQ0  Timer */
ISR_NOERR  33   /* IRQ1  Keyboard */
ISR_NOERR  34   /* IRQ2  Cascade */
ISR_NOERR  35   /* IRQ3  COM2 */
ISR_NOERR  36   /* IRQ4  COM1 */
ISR_NOERR  37   /* IRQ5  LPT2 */
ISR_NOERR  38   /* IRQ6  Floppy */
ISR_NOERR  39   /* IRQ7  LPT1 */
ISR_NOERR  40   /* IRQ8  CMOS RTC */
ISR_NOERR  41   /* IRQ9  Free */
ISR_NOERR  42   /* IRQ10 */
ISR_NOERR  43   /* IRQ11 */
ISR_NOERR  44   /* IRQ12  PS/2 Mouse */
ISR_NOERR  45   /* IRQ13  FPU */
ISR_NOERR  46   /* IRQ14  Primary ATA */
ISR_NOERR  47   /* IRQ15  Secondary ATA */

/* ── Syscall vector 128 (int 0x80) ─────────────────────────────────── */
ISR_NOERR 128

/* ── Default catch-all ───────────────────────────────────────────────── */
.global isr_default
isr_default:
    pushq $0
    pushq $255
    jmp   isr_common

/* ── Common dispatcher ───────────────────────────────────────────────── */
isr_common:
    /* Save caller-saved + callee-saved registers */
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

    cld

    /*
     * After 15 pushq the stack grew by 120 bytes.
     * vector     is at RSP + 120
     * error_code is at RSP + 128
     */
    movq 120(%rsp), %rdi    /* arg1 = vector      */
    movq 128(%rsp), %rsi    /* arg2 = error_code  */

    /* Dispatch to C-level handler which handles exceptions and IRQs */
    call isr_common_c

    /* Restore registers */
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

    addq $16, %rsp   /* discard vector + error_code */
    iretq

    .section .note.GNU-stack,"",@progbits
