#ifndef NANAOS_PIT_H
#define NANAOS_PIT_H

#include <stdint.h>

/* Initialize PIT with given frequency (Hz) */
void pit_init(uint32_t frequency);

/* Called from IRQ handler on each tick */
void pit_on_interrupt(void);

/* Read ticks since boot */
uint64_t pit_get_ticks(void);

/* Uptime in seconds (approx) */
uint64_t uptime_seconds(void);

#endif /* NANAOS_PIT_H */
