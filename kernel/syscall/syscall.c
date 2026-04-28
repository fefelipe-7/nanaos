#include "syscall/syscall.h"
#include "drivers/serial/com1.h"
#include "terminal/vga.h"
#include "drivers/timer/pit.h"
#include "log/logger.h"
#include "fs/vfs.h"
#include "process/process.h"
#include <stdint.h>
#include <stddef.h>

typedef uint64_t (*syscall_fn_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static process_t *sys_current_process(void) {
    process_t *p = process_get_current();
    if (p) return p;
    static process_t kernel_io_proc;
    static int init = 0;
    if (!init) {
        for (int i = 0; i < PROC_MAX_FDS; ++i) {
            kernel_io_proc.fds[i].used = 0;
            kernel_io_proc.fds[i].fd = i;
            kernel_io_proc.fds[i].offset = 0;
            kernel_io_proc.fds[i].path[0] = '\0';
        }
        kernel_io_proc.fds[0].used = 1;
        kernel_io_proc.fds[1].used = 1;
        kernel_io_proc.fds[2].used = 1;
        init = 1;
    }
    return &kernel_io_proc;
}

static process_fd_t *sys_get_fd(process_t *p, int fd) {
    if (!p || fd < 0 || fd >= PROC_MAX_FDS) return NULL;
    if (!p->fds[fd].used) return NULL;
    return &p->fds[fd];
}

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
    if (!buf || len == 0) return 0;
    process_t *p = sys_current_process();
    if (!p) return (uint64_t)-1;

    if (fd == 0) {
        /* stdin ainda não possui buffer de linha bloqueante */
        return 0;
    }

    process_fd_t *f = sys_get_fd(p, (int)fd);
    if (!f) return (uint64_t)-1;
    size_t file_len = 0;
    const char *data = vfs_read_file(f->path, &file_len, vfs_get_root());
    if (!data) return (uint64_t)-1;
    if (f->offset >= file_len) return 0;
    uint64_t avail = (uint64_t)file_len - f->offset;
    uint64_t to_read = (len < avail) ? len : avail;
    for (uint64_t i = 0; i < to_read; ++i) {
        buf[i] = data[f->offset + i];
    }
    f->offset += to_read;
    return to_read;
}

static uint64_t sys_get_ticks_impl(void) {
    return pit_get_ticks();
}

static uint64_t sys_exit_impl(int status) {
    log_info("[SYS] exit called");
    process_exit_current_status(status);
    /* Should not return, but return 0 just in case */
    return 0;
}

/* placeholders */
static uint64_t sys_open_impl(const char *path, int flags) {
    (void)flags;
    if (!path || path[0] == '\0') return (uint64_t)-1;
    process_t *p = sys_current_process();
    if (!p) return (uint64_t)-1;
    size_t file_len = 0;
    if (!vfs_read_file(path, &file_len, vfs_get_root())) return (uint64_t)-1;
    for (int i = 3; i < PROC_MAX_FDS; ++i) {
        if (!p->fds[i].used) {
            p->fds[i].used = 1;
            p->fds[i].offset = 0;
            size_t j = 0;
            while (path[j] && j < sizeof(p->fds[i].path) - 1) {
                p->fds[i].path[j] = path[j];
                ++j;
            }
            p->fds[i].path[j] = '\0';
            return (uint64_t)i;
        }
    }
    return (uint64_t)-1;
}

static uint64_t sys_close_impl(int fd) {
    process_t *p = sys_current_process();
    if (!p || fd < 3 || fd >= PROC_MAX_FDS) return (uint64_t)-1;
    p->fds[fd].used = 0;
    p->fds[fd].offset = 0;
    p->fds[fd].path[0] = '\0';
    return 0;
}

static uint64_t sys_lseek_impl(int fd, uint64_t off) {
    process_t *p = sys_current_process();
    process_fd_t *f = sys_get_fd(p, fd);
    if (!f) return (uint64_t)-1;
    f->offset = off;
    return off;
}

static uint64_t sys_getpid_impl(void) {
    process_t *p = sys_current_process();
    if (!p) return 0;
    return p->pid;
}

static uint64_t sys_sleep_impl(uint64_t ms) {
    uint64_t start = pit_get_ticks();
    uint64_t hz = 100;
    uint64_t wait_ticks = (ms * hz + 999) / 1000;
    if (wait_ticks == 0) wait_ticks = 1;
    while ((pit_get_ticks() - start) < wait_ticks) {
        asm volatile("hlt");
    }
    return 0;
}

static uint64_t sys_uptime_impl(void) {
    return uptime_seconds();
}

static uint64_t sys_write_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a4; (void)a5; (void)a6;
    return sys_write_impl(a1, (const char *)a2, a3);
}

static uint64_t sys_read_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a4; (void)a5; (void)a6;
    return sys_read_impl(a1, (char *)a2, a3);
}

static uint64_t sys_get_ticks_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return sys_get_ticks_impl();
}

static uint64_t sys_exit_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return sys_exit_impl((int)a1);
}

static uint64_t sys_open_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a3; (void)a4; (void)a5; (void)a6;
    return sys_open_impl((const char *)a1, (int)a2);
}

static uint64_t sys_close_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return sys_close_impl((int)a1);
}

static uint64_t sys_lseek_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a3; (void)a4; (void)a5; (void)a6;
    return sys_lseek_impl((int)a1, a2);
}

static uint64_t sys_getpid_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return sys_getpid_impl();
}

static uint64_t sys_sleep_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return sys_sleep_impl(a1);
}

static uint64_t sys_uptime_entry(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return sys_uptime_impl();
}

uint64_t syscall_dispatch(uint64_t num,
                          uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5, uint64_t a6) {
    static syscall_fn_t table[11] = {
        NULL,
        sys_write_entry,
        sys_read_entry,
        sys_get_ticks_entry,
        sys_exit_entry,
        sys_open_entry,
        sys_close_entry,
        sys_lseek_entry,
        sys_getpid_entry,
        sys_sleep_entry,
        sys_uptime_entry
    };
    if (num >= (sizeof(table) / sizeof(table[0])) || !table[num]) return (uint64_t)-1;
    return table[num](a1, a2, a3, a4, a5, a6);
}
