#ifndef NANAOS_THEME_H
#define NANAOS_THEME_H

#include <stdint.h>

typedef struct {
    uint32_t background;
    uint32_t surface;
    uint32_t surface_alt;
    uint32_t border;
    uint32_t text;
    uint32_t text_muted;
    uint32_t accent;
    uint32_t danger;
} theme_t;

const theme_t *theme_get_default(void);

#endif

