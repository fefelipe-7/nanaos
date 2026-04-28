#include "ui/nanawm/wm.h"
#include "drivers/framebuffer/fb.h"
#include "drivers/mouse/mouse.h"
#include "memory/heap.h"
#include "lib/string.h"
#include "log/logger.h"
#include <stdint.h>
#include <stddef.h>

#define WM_MAX_WINDOWS 8
#define WM_TITLEBAR_H 18

static wm_window_t g_windows[WM_MAX_WINDOWS];
static wm_window_t *g_active = NULL;

static uint32_t *g_backbuf = NULL;
static uint32_t g_bw = 0, g_bh = 0;

static int g_dragging = 0;
static int g_drag_offx = 0;
static int g_drag_offy = 0;
static int g_prev_btn = 0;

static inline void buf_put_pixel(uint32_t *buf, uint32_t pitch_px, int x, int y, uint32_t c) {
    if (!buf) return;
    if (x < 0 || y < 0) return;
    if ((uint32_t)x >= g_bw || (uint32_t)y >= g_bh) return;
    buf[(uint32_t)y * pitch_px + (uint32_t)x] = c;
}

static uint32_t lerp_u8(uint32_t a, uint32_t b, uint32_t t, uint32_t denom) {
    return (a * (denom - t) + b * t) / denom;
}

static uint32_t lerp_rgb(uint32_t c1, uint32_t c2, uint32_t t, uint32_t denom) {
    uint32_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint32_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    uint32_t r = lerp_u8(r1, r2, t, denom);
    uint32_t g = lerp_u8(g1, g2, t, denom);
    uint32_t b = lerp_u8(b1, b2, t, denom);
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

static void buf_fill_gradient(uint32_t *buf, uint32_t pitch_px, int w, int h, uint32_t top, uint32_t bottom) {
    if (!buf || w <= 0 || h <= 0) return;
    uint32_t denom = (h > 1) ? (uint32_t)(h - 1) : 1;
    for (int y = 0; y < h; ++y) {
        uint32_t c = lerp_rgb(top, bottom, (uint32_t)y, denom);
        for (int x = 0; x < w; ++x) buf[(uint32_t)y * pitch_px + (uint32_t)x] = c;
    }
}

static void buf_fill_rect(uint32_t *buf, uint32_t pitch_px, int x, int y, int w, int h, uint32_t c) {
    if (!buf || w <= 0 || h <= 0) return;
    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            buf_put_pixel(buf, pitch_px, x + xx, y + yy, c);
        }
    }
}

static void buf_draw_rect(uint32_t *buf, uint32_t pitch_px, int x, int y, int w, int h, uint32_t c) {
    if (!buf || w <= 0 || h <= 0) return;
    for (int xx = 0; xx < w; ++xx) {
        buf_put_pixel(buf, pitch_px, x + xx, y, c);
        buf_put_pixel(buf, pitch_px, x + xx, y + h - 1, c);
    }
    for (int yy = 0; yy < h; ++yy) {
        buf_put_pixel(buf, pitch_px, x, y + yy, c);
        buf_put_pixel(buf, pitch_px, x + w - 1, y + yy, c);
    }
}

static uint8_t glyph5x7(char c, int row) {
    switch (c) {
        case 'a': case 'A': { static const uint8_t r[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; return r[row]; }
        case 'b': case 'B': { static const uint8_t r[7] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}; return r[row]; }
        case 'c': case 'C': { static const uint8_t r[7] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}; return r[row]; }
        case 'd': case 'D': { static const uint8_t r[7] = {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}; return r[row]; }
        case 'e': case 'E': { static const uint8_t r[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; return r[row]; }
        case 'f': case 'F': { static const uint8_t r[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}; return r[row]; }
        case 'g': case 'G': { static const uint8_t r[7] = {0x0E,0x11,0x10,0x13,0x11,0x11,0x0E}; return r[row]; }
        case 'h': case 'H': { static const uint8_t r[7] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}; return r[row]; }
        case 'i': case 'I': { static const uint8_t r[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}; return r[row]; }
        case 'k': case 'K': { static const uint8_t r[7] = {0x11,0x12,0x14,0x18,0x14,0x12,0x11}; return r[row]; }
        case 'l': case 'L': { static const uint8_t r[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; return r[row]; }
        case 'm': case 'M': { static const uint8_t r[7] = {0x11,0x1B,0x15,0x11,0x11,0x11,0x11}; return r[row]; }
        case 'n': case 'N': { static const uint8_t r[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11}; return r[row]; }
        case 'o': case 'O': { static const uint8_t r[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; return r[row]; }
        case 'p': case 'P': { static const uint8_t r[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; return r[row]; }
        case 'r': case 'R': { static const uint8_t r[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; return r[row]; }
        case 's': case 'S': { static const uint8_t r[7] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; return r[row]; }
        case 't': case 'T': { static const uint8_t r[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; return r[row]; }
        case 'u': case 'U': { static const uint8_t r[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}; return r[row]; }
        case 'w': case 'W': { static const uint8_t r[7] = {0x11,0x11,0x11,0x11,0x15,0x1B,0x11}; return r[row]; }
        case 'x': case 'X': { static const uint8_t r[7] = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}; return r[row]; }
        case 'y': case 'Y': { static const uint8_t r[7] = {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}; return r[row]; }
        case 'z': case 'Z': { static const uint8_t r[7] = {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}; return r[row]; }
        case ':': { static const uint8_t r[7] = {0x00,0x04,0x00,0x00,0x04,0x00,0x00}; return r[row]; }
        case '-': { static const uint8_t r[7] = {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}; return r[row]; }
        case ' ': return 0x00;
        default: { static const uint8_t r[7] = {0x1F,0x11,0x02,0x04,0x08,0x11,0x1F}; return r[row]; }
    }
}

static void buf_draw_char(uint32_t *buf, uint32_t pitch_px, int x, int y, char c, uint32_t color) {
    for (int row = 0; row < 7; ++row) {
        uint8_t bits = glyph5x7(c, row);
        for (int col = 0; col < 5; ++col) {
            if (bits & (1U << (4 - col))) buf_put_pixel(buf, pitch_px, x + col, y + row, color);
        }
    }
}

static void buf_draw_string(uint32_t *buf, uint32_t pitch_px, int x, int y, const char *s, uint32_t color) {
    if (!s) return;
    int cx = x;
    for (size_t i = 0; s[i]; ++i) {
        buf_draw_char(buf, pitch_px, cx, y, s[i], color);
        cx += 6;
    }
}

static void copy_to_fb(void) {
    framebuffer_t *fb = fb_get();
    if (!fb_is_enabled() || !fb || !g_backbuf) return;
    uint8_t *dst = fb->addr;
    uint32_t *src = g_backbuf;
    for (uint32_t y = 0; y < fb->height; ++y) {
        uint32_t *row = src + y * g_bw;
        uint8_t *drow = dst + y * fb->pitch;
        for (uint32_t x = 0; x < fb->width; ++x) {
            uint32_t c = row[x];
            drow[x * 4 + 0] = (uint8_t)(c & 0xFF);
            drow[x * 4 + 1] = (uint8_t)((c >> 8) & 0xFF);
            drow[x * 4 + 2] = (uint8_t)((c >> 16) & 0xFF);
            drow[x * 4 + 3] = 0x00;
        }
    }
}

int wm_init(void) {
    if (!fb_is_enabled()) return -1;
    framebuffer_t *fb = fb_get();
    g_bw = fb->width;
    g_bh = fb->height;
    g_backbuf = (uint32_t *)kmalloc((size_t)g_bw * (size_t)g_bh * sizeof(uint32_t));
    if (!g_backbuf) return -2;
    for (int i = 0; i < WM_MAX_WINDOWS; ++i) g_windows[i].used = 0;
    g_active = NULL;
    log_info("[OK] nanawm ready");
    return 0;
}

static void win_fill_default(wm_window_t *w, uint32_t c) {
    if (!w || !w->buffer) return;
    for (int y = 0; y < w->h; ++y) {
        for (int x = 0; x < w->w; ++x) {
            w->buffer[y * w->w + x] = c;
        }
    }
}

wm_window_t *wm_create_window(int x, int y, int w, int h, const char *title, uint32_t flags) {
    for (int i = 0; i < WM_MAX_WINDOWS; ++i) {
        if (!g_windows[i].used) {
            wm_window_t *win = &g_windows[i];
            win->used = 1;
            win->x = x; win->y = y; win->w = w; win->h = h;
            win->active = 0;
            win->flags = flags;
            win->buffer = (uint32_t *)kmalloc((size_t)w * (size_t)h * sizeof(uint32_t));
            if (!win->buffer) { win->used = 0; return NULL; }
            for (int j = 0; j < (int)sizeof(win->title) - 1; ++j) win->title[j] = 0;
            if (title) {
                size_t j = 0;
                while (title[j] && j < sizeof(win->title) - 1) { win->title[j] = title[j]; ++j; }
                win->title[j] = '\0';
            } else {
                win->title[0] = 'w'; win->title[1] = 'i'; win->title[2] = 'n'; win->title[3] = '\0';
            }
            win_fill_default(win, 0xFF202830);
            wm_focus_window(win);
            return win;
        }
    }
    return NULL;
}

void wm_close_window(wm_window_t *win) {
    if (!win) return;
    win->used = 0;
    if (g_active == win) g_active = NULL;
}

void wm_focus_window(wm_window_t *win) {
    if (!win) return;
    for (int i = 0; i < WM_MAX_WINDOWS; ++i) g_windows[i].active = 0;
    win->active = 1;
    g_active = win;
}

static int point_in(int px, int py, int x, int y, int w, int h) {
    return (px >= x && py >= y && px < x + w && py < y + h);
}

static wm_window_t *pick_window_at(int x, int y) {
    /* simples: última usada/ativa tem prioridade; senão varre do fim */
    if (g_active && g_active->used && point_in(x, y, g_active->x, g_active->y, g_active->w, g_active->h)) return g_active;
    for (int i = WM_MAX_WINDOWS - 1; i >= 0; --i) {
        if (g_windows[i].used && point_in(x, y, g_windows[i].x, g_windows[i].y, g_windows[i].w, g_windows[i].h)) return &g_windows[i];
    }
    return NULL;
}

void wm_handle_mouse(int x, int y, int buttons) {
    framebuffer_t *fb = fb_get();
    if (!fb_is_enabled() || !fb) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > (int)fb->width - 1) x = (int)fb->width - 1;
    if (y > (int)fb->height - 1) y = (int)fb->height - 1;

    int left = (buttons & 0x1);
    int left_down_edge = (left && !g_prev_btn);
    int left_up_edge = (!left && g_prev_btn);

    if (left_down_edge) {
        wm_window_t *w = pick_window_at(x, y);
        if (w) {
            wm_focus_window(w);
            /* botão fechar (top right) */
            int cx = w->x + w->w - 18;
            int cy = w->y + 2;
            if (point_in(x, y, cx, cy, 14, 14)) {
                wm_close_window(w);
            } else if ((w->flags & WM_WIN_MOVABLE) && point_in(x, y, w->x, w->y, w->w, WM_TITLEBAR_H)) {
                g_dragging = 1;
                g_drag_offx = x - w->x;
                g_drag_offy = y - w->y;
            }
        }
    }

    if (g_dragging && g_active && g_active->used && left) {
        g_active->x = x - g_drag_offx;
        g_active->y = y - g_drag_offy;
        if (g_active->x < 0) g_active->x = 0;
        if (g_active->y < 0) g_active->y = 0;
    }

    if (left_up_edge) {
        g_dragging = 0;
    }

    g_prev_btn = left;
}

void wm_handle_key(int keycode_ascii) {
    if (!g_active || !g_active->used) return;
    /* demo: escreve tecla no título se for letra */
    (void)keycode_ascii;
}

static void composite_window(wm_window_t *w) {
    if (!w || !w->used) return;
    uint32_t border = w->active ? 0xFF6CA8FF : 0xFF2C3440;
    uint32_t title_bg = w->active ? 0xFF1D2A3A : 0xFF151C26;
    uint32_t title_fg = 0xFFEAF2FF;
    uint32_t close_bg = 0xFFCC4444;

    /* frame */
    /* shadow */
    buf_fill_rect(g_backbuf, g_bw, w->x + 4, w->y + 6, w->w, w->h, 0xFF0A0F16);
    buf_fill_rect(g_backbuf, g_bw, w->x, w->y, w->w, w->h, 0xFF10161F);
    buf_draw_rect(g_backbuf, g_bw, w->x, w->y, w->w, w->h, border);
    buf_fill_rect(g_backbuf, g_bw, w->x + 1, w->y + 1, w->w - 2, WM_TITLEBAR_H - 2, title_bg);
    buf_draw_string(g_backbuf, g_bw, w->x + 8, w->y + 6, w->title, title_fg);
    /* close button */
    buf_fill_rect(g_backbuf, g_bw, w->x + w->w - 18, w->y + 2, 14, 14, close_bg);
    buf_draw_char(g_backbuf, g_bw, w->x + w->w - 14, w->y + 6, 'x', 0xFFFFFFFF);

    /* content blit (abaixo da titlebar) */
    int content_y = w->y + WM_TITLEBAR_H;
    int content_h = w->h - WM_TITLEBAR_H;
    for (int yy = 0; yy < content_h; ++yy) {
        for (int xx = 0; xx < w->w; ++xx) {
            uint32_t c = w->buffer[(yy + 0) * w->w + xx];
            buf_put_pixel(g_backbuf, g_bw, w->x + xx, content_y + yy, c);
        }
    }
}

static void composite_cursor(void) {
    int mx = 0, my = 0;
    mouse_get_position(&mx, &my);
    /* cursor em formato de seta (12x16) */
    static const uint16_t shape[16] = {
        0b100000000000,
        0b110000000000,
        0b111000000000,
        0b111100000000,
        0b111110000000,
        0b111111000000,
        0b111111100000,
        0b111111110000,
        0b111100000000,
        0b110110000000,
        0b100011000000,
        0b000011000000,
        0b000001100000,
        0b000001100000,
        0b000000110000,
        0b000000110000,
    };
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 12; ++x) {
            if (shape[y] & (1U << (11 - x))) {
                buf_put_pixel(g_backbuf, g_bw, mx + x, my + y, 0xFFFFFFFF);
                if (x == 0 || y == 0) buf_put_pixel(g_backbuf, g_bw, mx + x - 1, my + y - 1, 0xFF000000);
            }
        }
    }
}

void wm_composite(void) {
    if (!g_backbuf) return;
    /* background */
    buf_fill_gradient(g_backbuf, g_bw, (int)g_bw, (int)g_bh, 0xFF0B1220, 0xFF0E2336);
    /* nanaura chrome (top bar + dock) */
    buf_fill_rect(g_backbuf, g_bw, 0, 0, (int)g_bw, 26, 0xFF111827);
    buf_draw_string(g_backbuf, g_bw, 12, 9, "nanaos", 0xFFEAF2FF);
    buf_draw_string(g_backbuf, g_bw, (int)g_bw - 58, 9, "12:00", 0xFFBFD0E6);

    int dock_w = 240;
    int dock_h = 52;
    int dock_x = ((int)g_bw - dock_w) / 2;
    int dock_y = (int)g_bh - dock_h - 10;
    /* sombra do dock */
    buf_fill_rect(g_backbuf, g_bw, dock_x + 2, dock_y + 3, dock_w, dock_h, 0xFF0A0F16);
    buf_fill_rect(g_backbuf, g_bw, dock_x, dock_y, dock_w, dock_h, 0xFF151F2B);
    buf_draw_rect(g_backbuf, g_bw, dock_x, dock_y, dock_w, dock_h, 0xFF2A3A4F);
    /* 3 ícones */
    for (int i = 0; i < 3; ++i) {
        int ix = dock_x + 16 + i * 72;
        int iy = dock_y + 10;
        buf_fill_rect(g_backbuf, g_bw, ix + 1, iy + 2, 32, 32, 0xFF0B0F16);
        buf_fill_rect(g_backbuf, g_bw, ix, iy, 32, 32, 0xFF223145);
        buf_draw_rect(g_backbuf, g_bw, ix, iy, 32, 32, 0xFF6CA8FF);
        buf_draw_char(g_backbuf, g_bw, ix + 13, iy + 12, (i == 0) ? 't' : (i == 1) ? 'f' : 's', 0xFFEAF2FF);
    }
    /* windows */
    for (int i = 0; i < WM_MAX_WINDOWS; ++i) {
        if (g_windows[i].used && &g_windows[i] != g_active) composite_window(&g_windows[i]);
    }
    if (g_active && g_active->used) composite_window(g_active);
    /* cursor por último */
    composite_cursor();
    copy_to_fb();
}

