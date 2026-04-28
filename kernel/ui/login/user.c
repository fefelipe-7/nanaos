#include "ui/login/user.h"

static const user_t g_user = {
    .username = "felipe",
    .display_name = "Felipe",
    .password_hash = "PLAINTEXT:nana",
    .avatar_color = 0xFF6CA8FF,
};

const user_t *user_get_default(void) {
    return &g_user;
}

