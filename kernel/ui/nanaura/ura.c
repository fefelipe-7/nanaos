#include "ui/nanaura/ura.h"
#include "ui/nanawm/wm.h"
#include "drivers/framebuffer/fb.h"
#include "core/events.h"
#include "log/logger.h"
#include <stdint.h>

static int g_launcher = 0;
static int g_mouse_x = 0;
static int g_mouse_y = 0;
static int g_mouse_btn = 0;
static int g_prev_left = 0;

static void create_default_windows(void) {
    (void)wm_create_window(80, 80, 360, 240, "terminal", WM_WIN_MOVABLE);
    (void)wm_create_window(180, 140, 320, 220, "files", WM_WIN_MOVABLE);
    (void)wm_create_window(300, 110, 280, 200, "settings", WM_WIN_MOVABLE);
}

int nanaura_init(void *multiboot_info) {
    (void)multiboot_info;
    if (!fb_is_enabled()) return -1;
    if (wm_init() != 0) return -2;
    create_default_windows();
    log_info("nanaos desktop loaded");
    return 0;
}

static void nanaura_handle_event(event_t ev) {
    if (ev.type == EVENT_MOUSE) {
        g_mouse_x = ev.a;
        g_mouse_y = ev.b;
        g_mouse_btn = ev.c;
        int left = (g_mouse_btn & 0x1);
        int left_edge = (left && !g_prev_left);

        /* Dock click abre apps (3 ícones) */
        if (left_edge) {
            framebuffer_t *fb = fb_get();
            int dock_w = 240;
            int dock_h = 52;
            int dock_x = ((int)fb->width - dock_w) / 2;
            int dock_y = (int)fb->height - dock_h - 10;
            for (int i = 0; i < 3; ++i) {
                int ix = dock_x + 16 + i * 72;
                int iy = dock_y + 10;
                if (g_mouse_x >= ix && g_mouse_x < ix + 32 && g_mouse_y >= iy && g_mouse_y < iy + 32) {
                    if (i == 0) (void)wm_create_window(90, 70, 380, 260, "terminal", WM_WIN_MOVABLE);
                    if (i == 1) (void)wm_create_window(160, 120, 340, 240, "files", WM_WIN_MOVABLE);
                    if (i == 2) (void)wm_create_window(240, 90, 320, 220, "settings", WM_WIN_MOVABLE);
                }
            }
        }

        wm_handle_mouse(g_mouse_x, g_mouse_y, g_mouse_btn);
        g_prev_left = left;
    } else if (ev.type == EVENT_KEYBOARD) {
        int c = ev.a;
        /* toggle launcher (spotlight-like) */
        if (c == 'l' || c == 'L') {
            g_launcher = !g_launcher;
        } else {
            wm_handle_key(c);
        }
    }
}

void nanaura_run(void) {
    if (!fb_is_enabled()) return;
    for (;;) {
        event_t ev;
        while (events_dequeue(&ev) == 0) nanaura_handle_event(ev);
        /* repaint simples: sempre compõe */
        wm_composite();
        asm volatile("hlt");
    }
}

