#include "arch/x86_64/pic/pic.h"
#include "arch/x86_64/port/port.h"
#include <stdint.h>

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

void pic_remap(int offset1, int offset2) {
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    /* start initialization sequence (cascade mode) */
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);

    /* set vector offsets */
    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);

    /* tell Master about Slave at IRQ2 (0000 0100) */
    outb(PIC1_DATA, 0x04);
    /* tell Slave its cascade identity (0000 0010) */
    outb(PIC2_DATA, 0x02);

    /* set 8086 mode */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* restore saved masks */
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    if (irq < 8) {
        port = PIC1_DATA;
        value = inb(port) | (1 << irq);
        outb(port, value);
    } else {
        port = PIC2_DATA;
        uint8_t rel = irq - 8;
        value = inb(port) | (1 << rel);
        outb(port, value);
    }
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    if (irq < 8) {
        port = PIC1_DATA;
        value = inb(port) & ~(1 << irq);
        outb(port, value);
    } else {
        port = PIC2_DATA;
        uint8_t rel = irq - 8;
        value = inb(port) & ~(1 << rel);
        outb(port, value);
    }
}

void pic_init(void) {
    /* Remap PIC to 0x20..0x2F */
    pic_remap(0x20, 0x28);

    /* Mask all IRQs, then enable timer (0), keyboard (1) and mouse (12).
     * IMPORTANT: unmask IRQ2 (cascade) so slave IRQs (8-15) can reach master. */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    pic_clear_mask(0); /* timer */
    pic_clear_mask(1); /* keyboard */
    pic_clear_mask(2); /* cascade */
    pic_clear_mask(12); /* mouse */
}
