#include "ui/theme/theme.h"

static const theme_t g_theme = {
    .background = 0xFF0E2336,
    .surface = 0xFF151F2B,
    .surface_alt = 0xFF111827,
    .border = 0xFF2A3A4F,
    .text = 0xFFEAF2FF,
    .text_muted = 0xFFBFD0E6,
    .accent = 0xFF6CA8FF,
    .danger = 0xFFCC4444,
};

const theme_t *theme_get_default(void) {
    return &g_theme;
}

