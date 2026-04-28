#include "ui/system/ui.h"
#include "ui/boot_splash/boot_splash.h"
#include "ui/login/login_screen.h"
#include "ui/nanaura/ura.h"
#include "drivers/framebuffer/fb.h"
#include "drivers/timer/pit.h"
#include <stddef.h>

static ui_state_t g_state = UI_STATE_BOOT_SPLASH;

static void busy_delay_ms(uint64_t ms) {
    uint64_t start = pit_get_ticks();
    uint64_t hz = 100;
    uint64_t wait = (ms * hz + 999) / 1000;
    if (wait == 0) wait = 1;
    while ((pit_get_ticks() - start) < wait) {
        asm volatile("hlt");
    }
}

int ui_system_init(void *multiboot_info) {
    (void)multiboot_info;
    if (!fb_is_enabled()) return -1;
    g_state = UI_STATE_BOOT_SPLASH;

    boot_splash_init();
    boot_splash_set_status("initializing nanacore...");
    boot_splash_set_progress(10);
    boot_splash_render();
    busy_delay_ms(120);

    boot_splash_set_status("loading drivers...");
    boot_splash_set_progress(35);
    boot_splash_render();
    busy_delay_ms(120);

    boot_splash_set_status("starting services...");
    boot_splash_set_progress(60);
    boot_splash_render();
    busy_delay_ms(120);

    boot_splash_set_status("loading nanaura...");
    boot_splash_set_progress(80);
    boot_splash_render();
    busy_delay_ms(120);

    boot_splash_finish();
    busy_delay_ms(200);

    g_state = UI_STATE_LOGIN;
    login_screen_init();
    return 0;
}

static void ui_render(void) {
    if (g_state == UI_STATE_BOOT_SPLASH) {
        boot_splash_render();
    } else if (g_state == UI_STATE_LOGIN) {
        login_screen_render();
    } else {
        /* desktop render é feito pelo compositor do nanawm dentro do loop do nanaura */
    }
}

void ui_system_run(void) {
    if (!fb_is_enabled()) return;
    for (;;) {
        event_t ev;
        while (events_dequeue(&ev) == 0) {
            if (g_state == UI_STATE_LOGIN) {
                login_screen_handle_event(&ev);
                if (login_screen_is_done()) {
                    g_state = UI_STATE_DESKTOP;
                    /* entra no desktop */
                    nanaura_init(NULL);
                    nanaura_run();
                }
            }
        }
        ui_render();
        asm volatile("hlt");
    }
}

