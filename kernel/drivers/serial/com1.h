#ifndef NANACORE_COM1_H
#define NANACORE_COM1_H

#include <stdint.h>

void serial_init(void);
void serial_write_char(char c);
void serial_write_string(const char *str);
void serial_write(const char *str);

#endif // NANACORE_COM1_H
