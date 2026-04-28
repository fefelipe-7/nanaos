#include "drivers/timer/pit.h"
#include "arch/x86_64/port/port.h"
#include "core/events.h"
#include <stdint.h>

static volatile uint64_t pit_ticks = 0;
static uint32_t pit_frequency = 0;

void pit_init(uint32_t frequency) {
    pit_frequency = frequency;
    pit_ticks = 0;

    /* PIT runs at 1.19318 MHz */
    uint32_t divisor = 1193180 / frequency;

    /* Command port: channel 0, access mode lobyte/hibyte, mode 3 (square wave) */
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void pit_on_interrupt(void) {
    pit_ticks++;
    if ((pit_ticks % 10) == 0) {
        event_t ev;
        ev.type = EVENT_TIMER;
        ev.a = (int32_t)pit_ticks;
        ev.b = 0;
        ev.c = 0;
        (void)events_enqueue(ev);
    }
    /* Notify scheduler (no printing here) */
    extern void scheduler_tick(void);
    scheduler_tick();
}

uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

uint64_t uptime_seconds(void) {
    if (pit_frequency == 0)
        return 0;
    return pit_ticks / pit_frequency;
}
