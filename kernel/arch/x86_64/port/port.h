#ifndef NANAOS_PORT_H
#define NANAOS_PORT_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/* Small I/O delay — write to unused port 0x80 */
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif /* NANAOS_PORT_H */
