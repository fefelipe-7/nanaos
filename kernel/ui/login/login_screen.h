#ifndef NANAOS_LOGIN_SCREEN_H
#define NANAOS_LOGIN_SCREEN_H

#include "core/events.h"

void login_screen_init(void);
void login_screen_render(void);
void login_screen_handle_event(const event_t *event);
int login_screen_authenticate(void);
void login_screen_transition_to_desktop(void);
int login_screen_is_done(void);

#endif

