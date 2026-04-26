.section .text
.global _start
.extern user_main
_start:
    xor %rbp, %rbp
    mov $0, %rdi
    call user_main
    /* If user_main returns, use its value as exit code */
    mov %rax, %rdi
    mov $4, %rax
    int $0x80
    hlt
