#include "com1.h"
#include "arch/x86_64/port/port.h"

#define SERIAL_PORT_COM1 0x3F8

static inline int is_transmit_empty(void) {
    return inb(SERIAL_PORT_COM1 + 5) & 0x20;
}

void serial_init(void) {
    outb(SERIAL_PORT_COM1 + 1, 0x00); // Disable all interrupts
    outb(SERIAL_PORT_COM1 + 3, 0x80); // Enable DLAB
    outb(SERIAL_PORT_COM1 + 0, 0x03); // Set baud rate divisor to 3 (38400 baud)
    outb(SERIAL_PORT_COM1 + 1, 0x00);
    outb(SERIAL_PORT_COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT_COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT_COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

void serial_write_char(char c) {
    while (!is_transmit_empty()) {
        ;
    }
    outb(SERIAL_PORT_COM1, (uint8_t)c);
}

void serial_write_string(const char *str) {
    for (const char *p = str; *p; ++p) {
        if (*p == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*p);
    }
}
