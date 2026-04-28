#include "exceptions.h"
#include "terminal/vga.h"
#include "lib/string.h"
#include "process/process.h"
#include <stdint.h>

/* ── Human-readable names for vectors 0-21 ───────────────────────────── */
static const char *const exception_names[] = {
    [0]  = "Divide-by-Zero (#DE)",
    [1]  = "Debug (#DB)",
    [2]  = "Non-Maskable Interrupt",
    [3]  = "Breakpoint (#BP)",
    [4]  = "Overflow (#OF)",
    [5]  = "Bound Range Exceeded (#BR)",
    [6]  = "Invalid Opcode (#UD)",
    [7]  = "Device Not Available (#NM)",
    [8]  = "Double Fault (#DF)",
    [9]  = "Coprocessor Segment Overrun",
    [10] = "Invalid TSS (#TS)",
    [11] = "Segment Not Present (#NP)",
    [12] = "Stack-Segment Fault (#SS)",
    [13] = "General Protection Fault (#GP)",
    [14] = "Page Fault (#PF)",
    [15] = "Reserved",
    [16] = "x87 FPU Exception (#MF)",
    [17] = "Alignment Check (#AC)",
    [18] = "Machine Check (#MC)",
    [19] = "SIMD FP Exception (#XM)",
    [20] = "Virtualisation Exception (#VE)",
    [21] = "Control Protection (#CP)",
};

#define NAMES_COUNT ((uint64_t)(sizeof(exception_names) / sizeof(exception_names[0])))

/* ── Simple hex printer (no stdlib) ─────────────────────────────────────*/
static void print_hex64(uint64_t val) {
    char buf[19];
    buf[0]  = '0';
    buf[1]  = 'x';
    buf[18] = '\0';
    for (int i = 17; i >= 2; i--) {
        uint8_t nibble = val & 0xF;
        buf[i] = (char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
        val >>= 4;
    }
    terminal_write_string(buf);
}

/* ── Main dispatcher, called from interrupt_stubs.S ─────────────────── */
void exception_dispatch(uint64_t vector, uint64_t error_code) {
    if (vector == 14) {
        uint64_t cr2 = 0;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        process_t *cur = process_get_current();
        if (cur) {
            terminal_write_string("page fault in process ");
            terminal_write_string(cur->name);
            terminal_write_string("\nprocess killed\nsystem stable\n");
            process_exit_current_status(-14);
            return;
        }
        terminal_write_string("\n[PF] kernel page fault at ");
        print_hex64(cr2);
        terminal_write_string(" err=");
        print_hex64(error_code);
        terminal_write_string("\n");
    }
    /* Red-on-black for the panic banner */
    terminal_set_color(0x04, 0x00);   /* fg=red, bg=black */
    terminal_write_string("\n\n*** KERNEL EXCEPTION ***\n");

    terminal_set_color(0x0F, 0x00);   /* white */
    terminal_write_string("  Vector : ");
    char vbuf[12];
    itoa((int)vector, vbuf, 10);
    terminal_write_string(vbuf);
    terminal_write_string("  (");

    if (vector < NAMES_COUNT && exception_names[vector])
        terminal_write_string(exception_names[vector]);
    else if (vector == 255)
        terminal_write_string("Unhandled Interrupt");
    else
        terminal_write_string("Unknown");

    terminal_write_string(")\n");

    terminal_write_string("  ErrCode: ");
    print_hex64(error_code);
    terminal_write_string("\n");

    terminal_set_color(0x0C, 0x00);   /* light red */
    terminal_write_string("\nSystem halted.\n");

    __asm__ volatile("cli");
    for (;;)
        __asm__ volatile("hlt");
}
