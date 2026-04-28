#include "ui/login/login_screen.h"
#include "ui/login/user.h"
#include "ui/theme/theme.h"
#include "drivers/framebuffer/fb.h"
#include "drivers/timer/pit.h"
#include "lib/string.h"

static char g_pass[32];
static int g_pass_len = 0;
static int g_error = 0;
static int g_done = 0;

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

static int point_in(int px, int py, int x, int y, int w, int h) {
    return (px >= x && py >= y && px < x + w && py < y + h);
}

void login_screen_init(void) {
    g_pass_len = 0;
    g_pass[0] = '\0';
    g_error = 0;
    g_done = 0;
    login_screen_render();
}

int login_screen_is_done(void) {
    return g_done;
}

int login_screen_authenticate(void) {
    /* protótipo inseguro: senha plaintext "nana" */
    const char *ok = "nana";
    if (g_pass_len == 4 && strcmp(g_pass, ok) == 0) return 1;
    return 0;
}

void login_screen_transition_to_desktop(void) {
    g_done = 1;
}

void login_screen_handle_event(const event_t *event) {
    if (!event || g_done) return;
    if (event->type == EVENT_KEYBOARD) {
        char c = (char)event->a;
        if (c == '\n') {
            if (login_screen_authenticate()) {
                login_screen_transition_to_desktop();
            } else {
                g_error = 1;
                g_pass_len = 0;
                g_pass[0] = '\0';
            }
        } else if (c == '\b') {
            if (g_pass_len > 0) {
                g_pass_len--;
                g_pass[g_pass_len] = '\0';
            }
        } else if (c >= 32 && c <= 126) {
            if (g_pass_len < (int)sizeof(g_pass) - 1) {
                g_pass[g_pass_len++] = c;
                g_pass[g_pass_len] = '\0';
            }
        }
    } else if (event->type == EVENT_MOUSE) {
        int mx = event->a;
        int my = event->b;
        int left = (event->c & 0x1);
        static int prev_left = 0;
        int edge = (left && !prev_left);
        prev_left = left;
        if (!edge) return;

        framebuffer_t *fb = fb_get();
        int cx = (int)fb->width / 2;
        int cy = (int)fb->height / 2;
        int card_w = 320;
        int card_h = 210;
        int card_x = cx - card_w / 2;
        int card_y = cy - card_h / 2;
        int btn_x = card_x + 60;
        int btn_y = card_y + 150;
        if (point_in(mx, my, btn_x, btn_y, 200, 26)) {
            if (login_screen_authenticate()) login_screen_transition_to_desktop();
            else { g_error = 1; g_pass_len = 0; g_pass[0] = '\0'; }
        }
    }
}

void login_screen_render(void) {
    if (!fb_is_enabled()) return;
    const theme_t *t = theme_get_default();
    framebuffer_t *fb = fb_get();
    fill_gradient(0xFF0B1220, t->background);

    int cx = (int)fb->width / 2;
    int cy = (int)fb->height / 2;
    fb_draw_string((uint32_t)(cx - 18), (uint32_t)(cy - 150), "nanaos", t->text);

    int card_w = 320;
    int card_h = 210;
    int card_x = cx - card_w / 2;
    int card_y = cy - card_h / 2;

    /* sombra e card */
    fb_fill_rect((uint32_t)(card_x + 4), (uint32_t)(card_y + 6), (uint32_t)card_w, (uint32_t)card_h, 0xFF0A0F16);
    fb_fill_rect((uint32_t)card_x, (uint32_t)card_y, (uint32_t)card_w, (uint32_t)card_h, t->surface);
    fb_draw_rect((uint32_t)card_x, (uint32_t)card_y, (uint32_t)card_w, (uint32_t)card_h, t->border);

    const user_t *u = user_get_default();
    /* avatar */
    fb_fill_rect((uint32_t)(card_x + 130), (uint32_t)(card_y + 18), 60, 60, u->avatar_color);
    fb_draw_rect((uint32_t)(card_x + 130), (uint32_t)(card_y + 18), 60, 60, t->border);

    fb_draw_string((uint32_t)(card_x + 132), (uint32_t)(card_y + 88), u->display_name, t->text);

    /* campo senha */
    fb_fill_rect((uint32_t)(card_x + 60), (uint32_t)(card_y + 118), 200, 22, t->surface_alt);
    fb_draw_rect((uint32_t)(card_x + 60), (uint32_t)(card_y + 118), 200, 22, t->border);
    char masked[32];
    int n = g_pass_len;
    if (n > (int)sizeof(masked) - 1) n = (int)sizeof(masked) - 1;
    for (int i = 0; i < n; ++i) masked[i] = '*';
    masked[n] = '\0';
    fb_draw_string((uint32_t)(card_x + 66), (uint32_t)(card_y + 124), masked, t->text_muted);

    /* botão entrar */
    fb_fill_rect((uint32_t)(card_x + 60), (uint32_t)(card_y + 150), 200, 26, t->accent);
    fb_draw_rect((uint32_t)(card_x + 60), (uint32_t)(card_y + 150), 200, 26, t->border);
    fb_draw_string((uint32_t)(card_x + 128), (uint32_t)(card_y + 158), "entrar", 0xFF0B1220);

    /* desligar / reiniciar */
    fb_draw_string((uint32_t)(cx - 60), (uint32_t)(card_y + card_h + 22), "desligar", t->text_muted);
    fb_draw_string((uint32_t)(cx + 20), (uint32_t)(card_y + card_h + 22), "reiniciar", t->text_muted);

    if (g_error) {
        uint64_t ticks = pit_get_ticks();
        if ((ticks / 10) % 2) {
            fb_draw_string((uint32_t)(card_x + 70), (uint32_t)(card_y + 184), "senha incorreta", t->danger);
        }
    }
}

