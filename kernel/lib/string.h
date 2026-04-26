#ifndef NANACORE_STRING_H
#define NANACORE_STRING_H

#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
void itoa(unsigned long value, char *buffer, size_t base);
int atoi(const char *str);

#endif // NANACORE_STRING_H
