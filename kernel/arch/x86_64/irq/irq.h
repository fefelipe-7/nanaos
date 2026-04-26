#ifndef NANAOS_IRQ_H
#define NANAOS_IRQ_H

#include <stdint.h>

/* Entry point called from assembly stubs. */
void isr_common_c(uint64_t vector, uint64_t error_code);

#endif /* NANAOS_IRQ_H */
