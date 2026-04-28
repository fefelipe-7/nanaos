#ifndef NANAOS_EVENTS_H
#define NANAOS_EVENTS_H

#include <stdint.h>

typedef enum {
    EVENT_NONE = 0,
    EVENT_KEYBOARD,
    EVENT_MOUSE,
    EVENT_TIMER
} event_type_t;

typedef struct event {
    event_type_t type;
    int32_t a;
    int32_t b;
    int32_t c;
} event_t;

void events_init(void);
int events_enqueue(event_t ev);
int events_dequeue(event_t *out);

#endif
