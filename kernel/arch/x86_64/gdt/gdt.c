#include "gdt.h"
#include <stdint.h>
#include <stddef.h>

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

/* GDT: null · kernel code · kernel data · user code · user data · TSS(16 bytes) */
static struct gdt_entry gdt[7];
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

struct tss64 {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed));

static struct tss64 tss;

static void set_tss_entry(int i, uint64_t base, uint32_t limit) {
    /* TSS is a system segment: access=0x89 (present, type=9), flags=0x00 */
    set_entry(i, (uint32_t)(base & 0xFFFFFFFF), limit, 0x89, 0x00);
    /* upper base bits go into the next entry */
    uint64_t base_hi = base >> 32;
    gdt[i + 1].limit_low = (uint16_t)((base_hi) & 0xFFFF);
    gdt[i + 1].base_low = (uint16_t)((base_hi >> 16) & 0xFFFF);
    gdt[i + 1].base_mid = 0;
    gdt[i + 1].access = 0;
    gdt[i + 1].flags_lim = 0;
    gdt[i + 1].base_high = 0;
}

void gdt_set_kernel_rsp0(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}

void gdt_init(void) {
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)gdt;

    /* 0x00 – null descriptor */
    set_entry(0, 0, 0, 0x00, 0x00);

    /* 0x08 – kernel code: present, ring0, code, read/exec, 64-bit (L=1) */
    set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);

    /* 0x10 – kernel data: present, ring0, data, read/write */
    set_entry(2, 0, 0xFFFFF, 0x92, 0xA0);

    /* 0x18 – user code: present, ring3, code */
    set_entry(3, 0, 0xFFFFF, 0xFA, 0xA0);

    /* 0x20 – user data: present, ring3, data */
    set_entry(4, 0, 0xFFFFF, 0xF2, 0xA0);

    /* TSS at 0x28 */
    for (uint64_t i = 0; i < (uint64_t)sizeof(tss); ++i) ((uint8_t *)&tss)[i] = 0;
    tss.iopb_offset = (uint16_t)sizeof(tss);
    set_tss_entry(5, (uint64_t)&tss, (uint32_t)(sizeof(tss) - 1));

    gdt_flush((uint64_t)&gdtr);

    /* Load TR with TSS selector (0x28) */
    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}
