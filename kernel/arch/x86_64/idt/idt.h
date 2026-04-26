#ifndef NANAOS_IDT_H
#define NANAOS_IDT_H

#include <stdint.h>

/* Initialise the IDT with all 256 gates and call lidt. */
void idt_init(void);

/* Set a single IDT gate.
 *   vector   — interrupt / exception number (0-255)
 *   handler  — address of the ASM stub
 *   selector — code segment selector (0x08 = kernel CS)
 *   flags    — type + attributes byte (0x8E = present, ring0, 64-bit interrupt gate)
 */
void idt_set_gate(uint8_t vector, uint64_t handler,
                  uint16_t selector, uint8_t flags);

#endif /* NANAOS_IDT_H */
