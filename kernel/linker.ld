OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

    /* Symbols to mark kernel physical start and end for PMM */
    __kernel_start = .;

    .text ALIGN(0x1000) : {
        *(.multiboot)
        *(.text*)
    }

    .rodata ALIGN(0x1000) : {
        *(.rodata*)
    }

    .data ALIGN(0x1000) : {
        *(.data*)
    }

    .bss ALIGN(0x1000) : {
        *(.bss*)
        *(COMMON)
    }

    __kernel_end = .;
}
