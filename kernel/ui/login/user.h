#ifndef NANAOS_USER_H
#define NANAOS_USER_H

#include <stdint.h>

typedef struct {
    char username[32];
    char display_name[64];
    char password_hash[64]; /* placeholder (inseguro) */
    uint32_t avatar_color;
} user_t;

const user_t *user_get_default(void);

#endif

