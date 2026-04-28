#include "drivers/framebuffer/fb.h"
#include "log/logger.h"
#include <stdint.h>
#include <stddef.h>

struct mb2_info { uint32_t total_size; uint32_t reserved; };
struct mb2_tag { uint32_t type; uint32_t size; };
struct mb2_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
};

#define MB2_TAG_TYPE_FB 8

static framebuffer_t g_fb;

framebuffer_t *fb_get(void) { return &g_fb; }
int fb_is_enabled(void) { return g_fb.enabled; }

int fb_init(uint64_t multiboot_info_addr) {
    g_fb.enabled = 0;
    if (!multiboot_info_addr) return -1;

    struct mb2_info *info = (struct mb2_info *)(uintptr_t)multiboot_info_addr;
    uint8_t *ptr = (uint8_t *)(uintptr_t)multiboot_info_addr + sizeof(struct mb2_info);
    uint8_t *end = (uint8_t *)(uintptr_t)multiboot_info_addr + info->total_size;

    while (ptr < end) {
        struct mb2_tag *tag = (struct mb2_tag *)ptr;
        if (tag->type == 0 && tag->size == 8) break;
        if (tag->type == MB2_TAG_TYPE_FB) {
            struct mb2_tag_framebuffer *fb = (struct mb2_tag_framebuffer *)ptr;
            g_fb.addr = (uint8_t *)(uintptr_t)fb->framebuffer_addr;
            g_fb.width = fb->framebuffer_width;
            g_fb.height = fb->framebuffer_height;
            g_fb.pitch = fb->framebuffer_pitch;
            g_fb.bpp = fb->framebuffer_bpp;
            g_fb.enabled = (g_fb.addr && g_fb.bpp >= 24) ? 1 : 0;
            break;
        }
        ptr += (tag->size + 7U) & ~7U;
    }

    if (g_fb.enabled) log_info("[OK] framebuffer ready");
    else log_info("[OK] framebuffer fallback VGA");
    return g_fb.enabled ? 0 : -1;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!g_fb.enabled || x >= g_fb.width || y >= g_fb.height) return;
    uint8_t *p = g_fb.addr + y * g_fb.pitch + x * (g_fb.bpp / 8);
    p[0] = (uint8_t)(color & 0xFF);
    p[1] = (uint8_t)((color >> 8) & 0xFF);
    p[2] = (uint8_t)((color >> 16) & 0xFF);
    if (g_fb.bpp == 32) p[3] = 0x00;
}

void fb_clear(uint32_t color) {
    if (!g_fb.enabled) return;
    for (uint32_t y = 0; y < g_fb.height; ++y) {
        for (uint32_t x = 0; x < g_fb.width; ++x) fb_put_pixel(x, y, color);
    }
}

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!g_fb.enabled || w == 0 || h == 0) return;
    for (uint32_t i = 0; i < w; ++i) {
        fb_put_pixel(x + i, y, color);
        fb_put_pixel(x + i, y + h - 1, color);
    }
    for (uint32_t i = 0; i < h; ++i) {
        fb_put_pixel(x, y + i, color);
        fb_put_pixel(x + w - 1, y + i, color);
    }
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!g_fb.enabled) return;
    for (uint32_t yy = 0; yy < h; ++yy) {
        for (uint32_t xx = 0; xx < w; ++xx) fb_put_pixel(x + xx, y + yy, color);
    }
}

static uint8_t glyph5x7(char c, int row) {
    /* Bit 4..0 representa coluna on/off. Fonte mínima para logs */
    switch (c) {
        case 'a': case 'A': { static const uint8_t r[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; return r[row]; }
        case 'e': case 'E': { static const uint8_t r[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; return r[row]; }
        case 'g': case 'G': { static const uint8_t r[7] = {0x0E,0x11,0x10,0x13,0x11,0x11,0x0E}; return r[row]; }
        case 'h': case 'H': { static const uint8_t r[7] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}; return r[row]; }
        case 'i': case 'I': { static const uint8_t r[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}; return r[row]; }
        case 'l': case 'L': { static const uint8_t r[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; return r[row]; }
        case 'n': case 'N': { static const uint8_t r[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11}; return r[row]; }
        case 'o': case 'O': { static const uint8_t r[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; return r[row]; }
        case 'p': case 'P': { static const uint8_t r[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; return r[row]; }
        case 'r': case 'R': { static const uint8_t r[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; return r[row]; }
        case 's': case 'S': { static const uint8_t r[7] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; return r[row]; }
        case 't': case 'T': { static const uint8_t r[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; return r[row]; }
        case 'z': case 'Z': { static const uint8_t r[7] = {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}; return r[row]; }
        case ' ': return 0x00;
        default: { static const uint8_t r[7] = {0x1F,0x11,0x02,0x04,0x08,0x11,0x1F}; return r[row]; }
    }
}

void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    if (!g_fb.enabled) return;
    for (int row = 0; row < 7; ++row) {
        uint8_t bits = glyph5x7(c, row);
        for (int col = 0; col < 5; ++col) {
            if (bits & (1U << (4 - col))) fb_put_pixel(x + (uint32_t)col, y + (uint32_t)row, color);
        }
    }
}

void fb_draw_string(uint32_t x, uint32_t y, const char *s, uint32_t color) {
    if (!s) return;
    uint32_t cx = x;
    for (size_t i = 0; s[i]; ++i) {
        fb_draw_char(cx, y, s[i], color);
        cx += 6;
    }
}
