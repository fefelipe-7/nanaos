#include "string.h"

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int)((unsigned char)*a - (unsigned char)*b);
}

int strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i] || a[i] == '\0' || b[i] == '\0') {
            return (int)((unsigned char)a[i] - (unsigned char)b[i]);
        }
    }
    return 0;
}

void itoa(unsigned long value, char *buffer, size_t base) {
    static const char digits[] = "0123456789";
    char temp[32];
    size_t length = 0;

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    while (value > 0) {
        temp[length++] = digits[value % base];
        value /= base;
    }

    for (size_t i = 0; i < length; ++i) {
        buffer[i] = temp[length - 1 - i];
    }
    buffer[length] = '\0';
}

int atoi(const char *str) {
    int sign = 1;
    int num = 0;
    /* skip leading whitespace */
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') ++str;
    if (*str == '-') { sign = -1; ++str; }
    else if (*str == '+') { ++str; }
    while (*str >= '0' && *str <= '9') {
        num = num * 10 + (*str - '0');
        ++str;
    }
    return num * sign;
}
