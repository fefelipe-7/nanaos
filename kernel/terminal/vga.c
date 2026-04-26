#include "vga.h"
#include <stddef.h>
#include <stdint.h>

static volatile uint16_t *const VGA_BUFFER = (volatile uint16_t *)0xB8000;
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

static inline uint16_t terminal_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void terminal_newline(void) {
    terminal_column = 0;
    if (++terminal_row == 25) {
        for (size_t y = 1; y < 25; y++) {
            for (size_t x = 0; x < 80; x++) {
                VGA_BUFFER[(y - 1) * 80 + x] = VGA_BUFFER[y * 80 + x];
            }
        }
        for (size_t x = 0; x < 80; x++) {
            VGA_BUFFER[(25 - 1) * 80 + x] = terminal_entry(' ', terminal_color);
        }
        terminal_row = 25 - 1;
    }
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x0F;
    for (size_t y = 0; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            VGA_BUFFER[y * 80 + x] = terminal_entry(' ', terminal_color);
        }
    }
}

void terminal_clear(void) {
    terminal_initialize();
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }

    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            VGA_BUFFER[terminal_row * 80 + terminal_column] = terminal_entry(' ', terminal_color);
        }
        return;
    }

    VGA_BUFFER[terminal_row * 80 + terminal_column] = terminal_entry(c, terminal_color);
    if (++terminal_column == 80) {
        terminal_newline();
    }
}

void terminal_write_string(const char *data) {
    for (const char *p = data; *p; ++p) {
        terminal_putchar(*p);
    }
}

void terminal_write(const char *data) {
    terminal_write_string(data);
}

void terminal_writeln(const char *data) {
    terminal_write_string(data);
    terminal_putchar('\n');
}

void terminal_set_color(uint8_t fg, uint8_t bg) {
    terminal_color = (uint8_t)((bg << 4) | (fg & 0x0F));
}
