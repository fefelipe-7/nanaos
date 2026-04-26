#include "panic.h"
#include "terminal/vga.h"
#include "drivers/serial/com1.h"

void panic(const char *message) {
    terminal_clear();
    terminal_write_string("PANIC: ");
    terminal_write_string(message);
    terminal_write_string("\n");

    serial_write_string("PANIC: ");
    serial_write_string(message);
    serial_write_string("\n");

    asm volatile("cli");
    while (1) {
        asm volatile("hlt");
    }
}
