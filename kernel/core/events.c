#include "core/events.h"

#define EVENT_QUEUE_CAP 128

static event_t q[EVENT_QUEUE_CAP];
static volatile uint32_t head = 0;
static volatile uint32_t tail = 0;

void events_init(void) {
    head = 0;
    tail = 0;
}

int events_enqueue(event_t ev) {
    uint32_t next = (tail + 1U) % EVENT_QUEUE_CAP;
    if (next == head) return -1;
    q[tail] = ev;
    tail = next;
    return 0;
}

int events_dequeue(event_t *out) {
    if (!out) return -1;
    if (head == tail) return -1;
    *out = q[head];
    head = (head + 1U) % EVENT_QUEUE_CAP;
    return 0;
}
