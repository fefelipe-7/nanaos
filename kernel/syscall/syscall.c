#include "syscall/syscall.h"
#include "drivers/serial/com1.h"
#include "terminal/vga.h"
#include "drivers/timer/pit.h"
#include "log/logger.h"
#include <stdint.h>
#include <stddef.h>

static uint64_t sys_write_impl(uint64_t fd, const char *buf, uint64_t len) {
    if (!buf) return (uint64_t)-1;
    if (fd == 1 || fd == 2) {
        /* write to serial + VGA */
        for (uint64_t i = 0; i < len; ++i) {
            char c = buf[i];
            if (c == '\n') {
                serial_write_char('\r');
            }
            serial_write_char(c);
            terminal_putchar(c);
        }
        return len;
    }
    /* other fds not supported yet */
    return (uint64_t)-1;
}

static uint64_t sys_read_impl(uint64_t fd, char *buf, uint64_t len) {
    (void)fd; (void)buf; (void)len;
    /* not implemented yet */
    return (uint64_t)-1;
}

static uint64_t sys_get_ticks_impl(void) {
    return pit_get_ticks();
}

static uint64_t sys_exit_impl(int status) {
    log_info("[SYS] exit called");
    extern void process_exit_current_status(int code);
    process_exit_current_status(status);
    /* Should not return, but return 0 just in case */
    return 0;
}

/* placeholders */
static uint64_t sys_open_impl(const char *path, int flags) {
    (void)path; (void)flags; return (uint64_t)-1;
}

static uint64_t sys_close_impl(int fd) {
    (void)fd; return (uint64_t)-1;
}

uint64_t syscall_dispatch(uint64_t num,
                          uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a4; (void)a5; (void)a6;
    switch (num) {
        case SYS_WRITE:
            return sys_write_impl(a1, (const char *)a2, a3);
        case SYS_READ:
            return sys_read_impl(a1, (char *)a2, a3);
        case SYS_GET_TICKS:
            return sys_get_ticks_impl();
        case SYS_EXIT:
            return sys_exit_impl((int)a1);
        case SYS_OPEN:
            return sys_open_impl((const char *)a1, (int)a2);
        case SYS_CLOSE:
            return sys_close_impl((int)a1);
        default:
            return (uint64_t)-1;
    }
}
