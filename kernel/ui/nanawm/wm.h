#ifndef NANAOS_WM_H
#define NANAOS_WM_H

#include <stdint.h>

typedef enum {
    WM_WIN_MOVABLE = 1 << 0,
    WM_WIN_RESIZABLE = 1 << 1
} wm_window_flags_t;

typedef struct wm_window {
    int used;
    int x, y;
    int w, h;
    int active;
    uint32_t flags;
    char title[32];
    uint32_t *buffer; /* ARGB8888 window-local buffer (w*h) */
} wm_window_t;

int wm_init(void);
wm_window_t *wm_create_window(int x, int y, int w, int h, const char *title, uint32_t flags);
void wm_close_window(wm_window_t *win);
void wm_focus_window(wm_window_t *win);
void wm_handle_mouse(int x, int y, int buttons);
void wm_handle_key(int keycode_ascii);
void wm_composite(void);

#endif
