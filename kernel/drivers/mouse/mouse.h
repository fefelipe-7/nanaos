#ifndef NANAOS_MOUSE_H
#define NANAOS_MOUSE_H

#include <stdint.h>

void mouse_init(void);
void mouse_on_interrupt(void);
int mouse_get_position(int *x, int *y);

#endif
