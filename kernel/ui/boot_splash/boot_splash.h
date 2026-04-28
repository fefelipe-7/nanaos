#ifndef NANAOS_BOOT_SPLASH_H
#define NANAOS_BOOT_SPLASH_H

#include <stdint.h>

void boot_splash_init(void);
void boot_splash_set_status(const char *text);
void boot_splash_set_progress(int percent);
void boot_splash_render(void);
void boot_splash_finish(void);

#endif

