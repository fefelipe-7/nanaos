#ifndef NANAOS_GDT_H
#define NANAOS_GDT_H

#include <stdint.h>

/* Initialise the GDT and reload all segment registers. */
void gdt_init(void);

/* Update kernel RSP0 used when entering from ring3 (TSS.rsp0). */
void gdt_set_kernel_rsp0(uint64_t rsp0);

#endif /* NANAOS_GDT_H */
