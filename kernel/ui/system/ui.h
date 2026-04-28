#ifndef NANAOS_UI_SYSTEM_H
#define NANAOS_UI_SYSTEM_H

#include <stdint.h>
#include "core/events.h"

typedef enum {
    UI_STATE_BOOT_SPLASH = 0,
    UI_STATE_LOGIN,
    UI_STATE_DESKTOP
} ui_state_t;

int ui_system_init(void *multiboot_info);
void ui_system_run(void);

#endif

