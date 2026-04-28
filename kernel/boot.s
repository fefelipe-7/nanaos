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
    /* Request framebuffer from GRUB (any size/depth acceptable) */
    .short 5      /* tag type: framebuffer */
    .short 0      /* flags */
    .long 20      /* size */
    .long 0       /* width */
    .long 0       /* height */
    .long 32      /* depth preference */
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

    /* PDPT[0..3] → pd0..pd3 (identity-map 0..4GiB using 2MiB pages) */
    mov  $boot_pd0, %eax
    or   $3, %eax
    movl %eax, boot_pdpt + 0

    mov  $boot_pd1, %eax
    or   $3, %eax
    movl %eax, boot_pdpt + 8

    mov  $boot_pd2, %eax
    or   $3, %eax
    movl %eax, boot_pdpt + 16

    mov  $boot_pd3, %eax
    or   $3, %eax
    movl %eax, boot_pdpt + 24

    /* Fill each PD with 512x 2MiB entries (1GiB per PD) */
    xor %ecx, %ecx              /* ecx = entry index (0..511) */
.fill_pd0:
    mov %ecx, %eax
    shl $21, %eax               /* eax = phys base low32 = idx * 2MiB */
    or  $0x83, %eax             /* PS|RW|P */
    movl %eax, boot_pd0(,%ecx,8)
    inc %ecx
    cmp $512, %ecx
    jne .fill_pd0

    xor %ecx, %ecx
.fill_pd1:
    mov %ecx, %eax
    shl $21, %eax
    add $0x40000000, %eax       /* +1GiB */
    or  $0x83, %eax
    movl %eax, boot_pd1(,%ecx,8)
    inc %ecx
    cmp $512, %ecx
    jne .fill_pd1

    xor %ecx, %ecx
.fill_pd2:
    mov %ecx, %eax
    shl $21, %eax
    add $0x80000000, %eax       /* +2GiB */
    or  $0x83, %eax
    movl %eax, boot_pd2(,%ecx,8)
    inc %ecx
    cmp $512, %ecx
    jne .fill_pd2

    xor %ecx, %ecx
.fill_pd3:
    mov %ecx, %eax
    shl $21, %eax
    add $0xC0000000, %eax       /* +3GiB */
    or  $0x83, %eax
    movl %eax, boot_pd3(,%ecx,8)
    inc %ecx
    cmp $512, %ecx
    jne .fill_pd3

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
boot_pd0:   .skip 4096
boot_pd1:   .skip 4096
boot_pd2:   .skip 4096
boot_pd3:   .skip 4096

.align 16
stack:      .skip 16384     /* 16 KB */
stack_top:

    .section .note.GNU-stack,"",@progbits
