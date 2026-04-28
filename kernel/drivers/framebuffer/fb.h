#ifndef NANAOS_FB_H
#define NANAOS_FB_H

#include <stdint.h>

typedef struct framebuffer {
    uint8_t *addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
    int enabled;
} framebuffer_t;

int fb_init(uint64_t multiboot_info_addr);
int fb_is_enabled(void);
framebuffer_t *fb_get(void);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_clear(uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t color);
void fb_draw_string(uint32_t x, uint32_t y, const char *s, uint32_t color);

#endif
