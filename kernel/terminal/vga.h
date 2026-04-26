#ifndef NANACORE_VGA_H
#define NANACORE_VGA_H

#include <stddef.h>
#include <stdint.h>

void terminal_initialize(void);
void terminal_clear(void);
void terminal_putchar(char c);
void terminal_write_string(const char *data);

/* Convenience wrappers requested in Phase 2 API */
void terminal_write(const char *data);
void terminal_writeln(const char *data);
void terminal_set_color(uint8_t fg, uint8_t bg);

#endif // NANACORE_VGA_H
