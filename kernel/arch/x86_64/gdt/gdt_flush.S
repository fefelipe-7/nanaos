/*
 * gdt_flush.S — load the new GDTR and reload all segment registers.
 *
 * Prototype (C):  void gdt_flush(uint64_t gdtr_ptr);
 *   rdi = pointer to the packed { uint16_t limit; uint64_t base; } struct.
 */

.section .text
.global gdt_flush
gdt_flush:
    lgdt (%rdi)

    /* Reload CS via a far return: push new CS then RIP, then lretq */
    pushq $0x08
    leaq  .reload_cs(%rip), %rax
    pushq %rax
    lretq

.reload_cs:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    xorw %ax, %ax
    movw %ax, %fs
    movw %ax, %gs
    ret

    .section .note.GNU-stack,"",@progbits
