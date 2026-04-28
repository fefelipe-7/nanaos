#include "drivers/mouse/mouse.h"
#include "arch/x86_64/port/port.h"
#include "core/events.h"
#include "log/logger.h"

static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];
static int mouse_x = 20;
static int mouse_y = 20;
static int mouse_btn = 0;

static void mouse_wait_write(void) {
    for (int i = 0; i < 100000; ++i) {
        if ((inb(0x64) & 0x02) == 0) return;
    }
}

static void mouse_wait_read(void) {
    for (int i = 0; i < 100000; ++i) {
        if (inb(0x64) & 0x01) return;
    }
}

static void mouse_write(uint8_t val) {
    mouse_wait_write();
    outb(0x64, 0xD4);
    mouse_wait_write();
    outb(0x60, val);
}

static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(0x60);
}

void mouse_init(void) {
    outb(0x64, 0xA8); /* enable auxiliary device */
    mouse_wait_write();
    outb(0x64, 0x20); /* get status byte */
    mouse_wait_read();
    uint8_t status = inb(0x60) | 0x02;
    mouse_wait_write();
    outb(0x64, 0x60); /* set status byte */
    mouse_wait_write();
    outb(0x60, status);

    mouse_write(0xF6); (void)mouse_read();
    mouse_write(0xF4); (void)mouse_read();
    log_info("[OK] mouse ready");
}

void mouse_on_interrupt(void) {
    uint8_t data = inb(0x60);
    if (mouse_cycle == 0 && (data & 0x08) == 0) return;
    mouse_packet[mouse_cycle++] = data;
    if (mouse_cycle < 3) return;
    mouse_cycle = 0;

    int dx = (int8_t)mouse_packet[1];
    int dy = (int8_t)mouse_packet[2];
    int buttons = mouse_packet[0] & 0x07;

    mouse_x += dx;
    mouse_y -= dy;
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    /* clamp será feito pelo compositor (nanawm) quando o framebuffer estiver ativo */

    mouse_btn = buttons;
    event_t ev;
    ev.type = EVENT_MOUSE;
    ev.a = mouse_x;
    ev.b = mouse_y;
    ev.c = mouse_btn;
    (void)events_enqueue(ev);
}

int mouse_get_position(int *x, int *y) {
    if (x) *x = mouse_x;
    if (y) *y = mouse_y;
    return 0;
}
