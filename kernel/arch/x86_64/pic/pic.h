#ifndef NANAOS_PIC_H
#define NANAOS_PIC_H

#include <stdint.h>

/* Initialize PICs (remap and mask/unmask defaults) */
void pic_init(void);

/* Remap PIC vectors to given offsets (master, slave) */
void pic_remap(int offset1, int offset2);

/* Send End Of Interrupt for given IRQ (0-15) */
void pic_send_eoi(uint8_t irq);

/* Mask/unmask individual IRQ lines */
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

#endif /* NANAOS_PIC_H */
