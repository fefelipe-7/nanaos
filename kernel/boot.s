/* boot.s — x86_64 kernel entry with Multiboot2 + long-mode transition
 *
 * GRUB delivers control in 32-bit protected mode (Multiboot2 arch=0).
 * We must:
 *   1. Build minimal identity-mapped page tables
 *   2. Enable PAE, set CR3, set EFER.LME, enable paging
 *   3. Far-jump into the 64-bit code segment
 *   4. Set up the kernel stack and call kernel_main
 */

.code32

/* ─── Multiboot2 header ─────────────────────────────────────────────── */
.section .multiboot
.align 8
multiboot_header:
    .long 0xE85250D6
    .long 0
    .long multiboot_header_end - multiboot_header
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))
    .align 8
    .long 0
    .long 8
multiboot_header_end:

/* ─── Bootstrap GDT (lives in .data so it's accessible in 32-bit mode) ─ */
.section .data
.align 8
boot_gdt:
    .quad 0x0000000000000000    /* 0x00 null */
    .quad 0x00AF9A000000FFFF    /* 0x08 64-bit code: L=1, P=1, DPL=0 */
    .quad 0x00AF92000000FFFF    /* 0x10 64-bit data: P=1, DPL=0, W=1  */
boot_gdt_end:

boot_gdt_ptr:
    .word boot_gdt_end - boot_gdt - 1
    .long boot_gdt              /* 32-bit base is fine here */

/* Store Multiboot2 info pointer (32-bit) here so 64-bit code can read it */
.align 8
multiboot_info_ptr:
    .long 0

/* ─── 32-bit entry ──────────────────────────────────────────────────── */
.section .text
.global _start
.type _start, @function
.code32
_start:
    cli

    /* temporary 32-bit stack (BSS stack is already safe here) */
    mov $stack_top, %esp
    /* Save Multiboot2 info pointer passed in %ebx into data area */
    movl %ebx, multiboot_info_ptr

    /* ── 1. Build identity-mapped page tables ─────────────────────── */
    /* PML4[0] → pdpt */
    mov  $boot_pdpt, %eax
    or   $3, %eax               /* P | RW */
    movl %eax, boot_pml4

    /* PDPT[0] → pd */
    mov  $boot_pd, %eax
    or   $3, %eax
    movl %eax, boot_pdpt

    /* PD[0..3]: four 2 MB huge pages → identity-map first 8 MB
       Covers: multiboot header (0x100000), VGA (0xB8000), stack (~0x104000) */
    movl $0x00000083, boot_pd + 0   /* 0x000000–0x1FFFFF | PS|RW|P */
    movl $0x00200083, boot_pd + 8   /* 0x200000–0x3FFFFF */
    movl $0x00400083, boot_pd + 16  /* 0x400000–0x5FFFFF */
    movl $0x00600083, boot_pd + 24  /* 0x600000–0x7FFFFF */

    /* ── 2. Load GDT ─────────────────────────────────────────────── */
    lgdt boot_gdt_ptr

    /* ── 3. Enable PAE (CR4.PAE = bit 5) ─────────────────────────── */
    mov  %cr4, %eax
    or   $0x20, %eax
    mov  %eax, %cr4

    /* ── 4. Point CR3 at PML4 ────────────────────────────────────── */
    mov  $boot_pml4, %eax
    mov  %eax, %cr3

    /* ── 5. Enable Long Mode in EFER (MSR 0xC0000080, bit 8) ──────── */
    mov  $0xC0000080, %ecx
    rdmsr
    or   $0x100, %eax
    wrmsr

    /* ── 6. Enable paging → activates long mode ───────────────────── */
    mov  %cr0, %eax
    or   $0x80000001, %eax      /* PG | PE */
    mov  %eax, %cr0

    /* ── 7. Far-jump into 64-bit code segment ─────────────────────── */
    ljmpl $0x08, $_start64

/* ─── 64-bit entry ──────────────────────────────────────────────────── */
.code64
_start64:
    mov  $0x10, %ax
    mov  %ax, %ds
    mov  %ax, %es
    mov  %ax, %ss
    xor  %ax, %ax
    mov  %ax, %fs
    mov  %ax, %gs

    movq $stack_top, %rsp

    /* Load multiboot info pointer (saved earlier in 32-bit) into %rdi */
    movl multiboot_info_ptr(%rip), %edi

    call kernel_main

.halt:
    hlt
    jmp  .halt

/* ─── BSS: page tables + kernel stack ──────────────────────────────── */
.section .bss

.align 4096
boot_pml4:  .skip 4096
boot_pdpt:  .skip 4096
boot_pd:    .skip 4096

.align 16
stack:      .skip 16384     /* 16 KB */
stack_top:

    .section .note.GNU-stack,"",@progbits
