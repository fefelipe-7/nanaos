#include "idt.h"
#include "arch/x86_64/exceptions/exceptions.h"
#include <stdint.h>

/* ── IDT entry (16 bytes) ─────────────────────────────────────────────── */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;        /* Interrupt Stack Table index (0 = legacy) */
    uint8_t  type_attr;  /* 0x8E = present, ring0, 64-bit interrupt gate */
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_descriptor {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_descriptor idtr;

/* ── ASM stubs declared in interrupt_stubs.S ─────────────────────────── */
extern void isr0(void);    /* #DE  Divide-by-Zero          (no error code) */
extern void isr6(void);    /* #UD  Invalid Opcode           (no error code) */
extern void isr8(void);    /* #DF  Double Fault             (error code = 0) */
extern void isr13(void);   /* #GP  General Protection Fault (error code)    */
extern void isr14(void);   /* #PF  Page Fault               (error code)    */
extern void isr_default(void); /* catch-all for unhandled vectors */
/* IRQ stubs (32..47) provided by interrupt_stubs.S */
extern void isr32(void);
extern void isr33(void);
extern void isr34(void);
extern void isr35(void);
extern void isr36(void);
extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);
/* Syscall stub */
extern void isr128(void);

void idt_set_gate(uint8_t v, uint64_t handler, uint16_t sel, uint8_t flags) {
    idt[v].offset_low  =  handler        & 0xFFFF;
    idt[v].selector    =  sel;
    idt[v].ist         =  0;
    idt[v].type_attr   =  flags;
    idt[v].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[v].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[v].zero        =  0;
}

void idt_init(void) {
    /* Fill every vector with the default stub first */
    for (int i = 0; i < 256; i++)
        idt_set_gate((uint8_t)i, (uint64_t)isr_default, 0x08, 0x8E);

    /* Override the five exception vectors we handle specifically */
    idt_set_gate( 0, (uint64_t)isr0,   0x08, 0x8E); /* #DE */
    idt_set_gate( 6, (uint64_t)isr6,   0x08, 0x8E); /* #UD */
    idt_set_gate( 8, (uint64_t)isr8,   0x08, 0x8E); /* #DF */
    idt_set_gate(13, (uint64_t)isr13,  0x08, 0x8E); /* #GP */
    idt_set_gate(14, (uint64_t)isr14,  0x08, 0x8E); /* #PF */

    /* IRQ gates: map hardware IRQs to vectors 32..47 */
    idt_set_gate(32, (uint64_t)isr32, 0x08, 0x8E);
    idt_set_gate(33, (uint64_t)isr33, 0x08, 0x8E);
    idt_set_gate(34, (uint64_t)isr34, 0x08, 0x8E);
    idt_set_gate(35, (uint64_t)isr35, 0x08, 0x8E);
    idt_set_gate(36, (uint64_t)isr36, 0x08, 0x8E);
    idt_set_gate(37, (uint64_t)isr37, 0x08, 0x8E);
    idt_set_gate(38, (uint64_t)isr38, 0x08, 0x8E);
    idt_set_gate(39, (uint64_t)isr39, 0x08, 0x8E);
    idt_set_gate(40, (uint64_t)isr40, 0x08, 0x8E);
    idt_set_gate(41, (uint64_t)isr41, 0x08, 0x8E);
    idt_set_gate(42, (uint64_t)isr42, 0x08, 0x8E);
    idt_set_gate(43, (uint64_t)isr43, 0x08, 0x8E);
    idt_set_gate(44, (uint64_t)isr44, 0x08, 0x8E);
    idt_set_gate(45, (uint64_t)isr45, 0x08, 0x8E);
    idt_set_gate(46, (uint64_t)isr46, 0x08, 0x8E);
    idt_set_gate(47, (uint64_t)isr47, 0x08, 0x8E);

    /* Syscall vector (int 0x80) */
    idt_set_gate(128, (uint64_t)isr128, 0x08, 0xEE); /* DPL=3 to allow user->int */

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)idt;

    __asm__ volatile("lidt %0" : : "m"(idtr));
}
