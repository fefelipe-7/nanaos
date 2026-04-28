#include "ui/boot_splash/boot_splash.h"
#include "ui/theme/theme.h"
#include "drivers/framebuffer/fb.h"
#include "drivers/timer/pit.h"

static const char *g_status = "initializing nanacore...";
static int g_progress = 0;

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

static void fill_gradient(uint32_t top, uint32_t bottom) {
    framebuffer_t *fb = fb_get();
    if (!fb_is_enabled() || !fb) return;
    uint32_t denom = (fb->height > 1) ? (fb->height - 1) : 1;
    for (uint32_t y = 0; y < fb->height; ++y) {
        uint32_t c = lerp_rgb(top, bottom, y, denom);
        for (uint32_t x = 0; x < fb->width; ++x) fb_put_pixel(x, y, c);
    }
}

static void draw_progress_bar(int cx, int cy, int w, int h, int percent, const theme_t *t) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    fb_fill_rect((uint32_t)cx, (uint32_t)cy, (uint32_t)w, (uint32_t)h, t->surface);
    fb_draw_rect((uint32_t)cx, (uint32_t)cy, (uint32_t)w, (uint32_t)h, t->border);
    int inner = w - 4;
    int fill = (inner * percent) / 100;
    fb_fill_rect((uint32_t)(cx + 2), (uint32_t)(cy + 2), (uint32_t)fill, (uint32_t)(h - 4), t->accent);
}

void boot_splash_init(void) {
    g_status = "initializing nanacore...";
    g_progress = 0;
    boot_splash_render();
}

void boot_splash_set_status(const char *text) {
    if (text) g_status = text;
}

void boot_splash_set_progress(int percent) {
    g_progress = percent;
}

void boot_splash_render(void) {
    if (!fb_is_enabled()) return;
    const theme_t *t = theme_get_default();
    framebuffer_t *fb = fb_get();
    fill_gradient(0xFF0B1220, t->background);

    int cx = (int)fb->width / 2;
    int cy = (int)fb->height / 2;
    fb_draw_string((uint32_t)(cx - 18), (uint32_t)(cy - 30), "nanaos", t->text);

    draw_progress_bar(cx - 140, cy + 10, 280, 16, g_progress, t);
    fb_draw_string((uint32_t)(cx - 140), (uint32_t)(cy + 34), g_status, t->text_muted);

    /* indicador leve (3 pontos animados) */
    uint64_t ticks = pit_get_ticks();
    int dots = (int)((ticks / 10) % 4);
    char d[5] = {0,0,0,0,0};
    for (int i = 0; i < dots; ++i) d[i] = '.';
    fb_draw_string((uint32_t)(cx + 40), (uint32_t)(cy - 30), d, t->text_muted);
}

void boot_splash_finish(void) {
    boot_splash_set_status("ready");
    boot_splash_set_progress(100);
    boot_splash_render();
}

