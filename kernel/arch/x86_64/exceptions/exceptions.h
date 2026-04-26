#ifndef NANAOS_EXCEPTIONS_H
#define NANAOS_EXCEPTIONS_H

#include <stdint.h>

/*
 * Called by isr_common (interrupt_stubs.S).
 *   vector     — CPU exception number (0-255)
 *   error_code — value pushed by CPU, or 0 for exceptions without one
 */
void exception_dispatch(uint64_t vector, uint64_t error_code);

#endif /* NANAOS_EXCEPTIONS_H */
