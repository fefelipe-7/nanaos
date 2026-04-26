#include "gdt.h"
#include <stdint.h>

/* ── GDT entry (8 bytes) ──────────────────────────────────────────────── */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;      /* P | DPL | S | Type */
    uint8_t  flags_lim;   /* G | DB | L | AVL | limit[19:16] */
    uint8_t  base_high;
} __attribute__((packed));

/* ── GDT descriptor (10 bytes, for lidt/lgdt) ─────────────────────────── */
struct gdt_descriptor {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* Three entries: null · kernel code · kernel data */
static struct gdt_entry gdt[3];
static struct gdt_descriptor gdtr;

static void set_entry(int i, uint32_t base, uint32_t limit,
                      uint8_t access, uint8_t flags)
{
    gdt[i].limit_low  =  limit        & 0xFFFF;
    gdt[i].base_low   =  base         & 0xFFFF;
    gdt[i].base_mid   = (base >> 16)  & 0xFF;
    gdt[i].access     =  access;
    gdt[i].flags_lim  = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[i].base_high  = (base >> 24)  & 0xFF;
}

/* Defined in gdt_flush.S — loads GDTR and far-returns into CS=0x08 */
extern void gdt_flush(uint64_t gdtr_ptr);

void gdt_init(void) {
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)gdt;

    /* 0x00 – null descriptor */
    set_entry(0, 0, 0, 0x00, 0x00);

    /* 0x08 – kernel code: present, ring0, code, read/exec, 64-bit (L=1) */
    set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);

    /* 0x10 – kernel data: present, ring0, data, read/write */
    set_entry(2, 0, 0xFFFFF, 0x92, 0xA0);

    gdt_flush((uint64_t)&gdtr);
}
