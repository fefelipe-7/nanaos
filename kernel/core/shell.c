#include "core/shell.h"
#include "terminal/vga.h"
#include "log/logger.h"
#include "drivers/timer/pit.h"
#include "lib/string.h"
#include "fs/vfs.h"
#include <stddef.h>
#include <stdint.h>
#include "process/process.h"

/* Simple line buffer for shell input */
static char linebuf[128];
static size_t linepos = 0;
static vfs_node_t *cwd = NULL;
static char cwd_path[256];

static void update_cwd_path(void) {
    if (cwd) vfs_get_path(cwd, cwd_path, sizeof(cwd_path));
    else {
        cwd_path[0] = '/'; cwd_path[1] = '\0';
    }
}

static void prompt(void) {
    terminal_write_string("\n> ");
}

void shell_init(void) {
    terminal_write_string("\nnanaos shell v0.1\n");
    /* set initial CWD to root */
    cwd = vfs_get_root();
    update_cwd_path();
    prompt();
}

static void handle_command(const char *cmd) {
    /* Ignore empty commands (user pressed Enter without typing) */
    if (cmd == NULL || cmd[0] == '\0')
        return;
    /* Parse command and optional argument */
    char name[32] = {0};
    char arg[128] = {0};
    size_t i = 0;
    while (cmd[i] == ' ') ++i; /* skip leading spaces */
    size_t j = i;
    while (cmd[j] && cmd[j] != ' ' && j - i < sizeof(name) - 1) ++j;
    size_t nlen = j - i;
    for (size_t k = 0; k < nlen; ++k) name[k] = cmd[i + k];
    /* rest */
    while (cmd[j] == ' ') ++j;
    size_t alen = 0;
    while (cmd[j] && alen < sizeof(arg) - 1) arg[alen++] = cmd[j++];

    if (strcmp(name, "help") == 0) {
        terminal_write_string("Commands: help clear version about uptime echo ls cat touch mkdir pwd cd meminfo\n");
    } else if (strcmp(cmd, "clear") == 0) {
        terminal_clear();
    } else if (strcmp(cmd, "version") == 0) {
        terminal_write_string("nanaos v0.1\n");
    } else if (strcmp(cmd, "about") == 0) {
        terminal_write_string("nanaos — simple educational kernel\n");
    } else if (strcmp(cmd, "uptime") == 0) {
        uint64_t s = uptime_seconds();
        char buf[32];
        itoa(s, buf, 10);
        terminal_write_string("Uptime: ");
        terminal_write_string(buf);
        terminal_write_string(" seconds\n");
    } else if (strcmp(name, "echo") == 0) {
        terminal_write_string(arg);
        terminal_write_string("\n");
    } else if (strcmp(name, "ls") == 0) {
        const char *p = arg[0] ? arg : ".";
        if (vfs_list_dir(p, cwd) < 0) terminal_write_string("ls: not a directory\n");
    } else if (strcmp(name, "cat") == 0) {
        if (!arg[0]) { terminal_write_string("cat: missing path\n"); }
        else {
            size_t len = 0;
            const char *d = vfs_read_file(arg, &len, cwd);
            if (!d) terminal_write_string("cat: file not found\n");
            else terminal_write_string(d);
        }
    } else if (strcmp(name, "touch") == 0) {
        if (!arg[0]) terminal_write_string("touch: missing path\n");
        else {
            if (!vfs_create(arg, 0, cwd)) terminal_write_string("touch: failed\n");
        }
    } else if (strcmp(name, "mkdir") == 0) {
        if (!arg[0]) terminal_write_string("mkdir: missing path\n");
        else {
            if (!vfs_create(arg, 1, cwd)) terminal_write_string("mkdir: failed\n");
        }
    } else if (strcmp(name, "pwd") == 0) {
        terminal_write_string(cwd_path);
        terminal_write_string("\n");
    } else if (strcmp(name, "cd") == 0) {
        if (!arg[0]) { cwd = vfs_get_root(); update_cwd_path(); }
        else {
            vfs_node_t *n = vfs_resolve(arg, cwd);
            if (!n) terminal_write_string("cd: not found\n");
            else {
                /* ensure directory */
                extern vfs_node_t *vfs_get_root(void);
                /* node type check: hack around opaque type by checking resolve of listing */
                /* we can attempt to list and if success assume directory */
                if (vfs_list_dir(arg, cwd) == 0) {
                    cwd = n;
                    update_cwd_path();
                } else {
                    terminal_write_string("cd: not a directory\n");
                }
            }
        }
    } else if (strcmp(cmd, "meminfo") == 0) {
        extern void print_meminfo(void);
        print_meminfo();
    } else if (strcmp(name, "exec") == 0) {
        if (!arg[0]) { terminal_write_string("exec: missing path\n"); }
        else {
            process_t *p = process_exec(arg, NULL, 1);
            if (!p) terminal_write_string("exec: failed\n");
            else {
                /* Wait for the process to reach TERMINATED state and report its exit code */
                for (;;) {
                    process_t *pp = process_find_by_pid(p->pid);
                    if (pp && pp->state == PROC_TERMINATED) {
                        char buf[32]; itoa(pp->exit_code, buf, 10);
                        terminal_write_string("process exited with code ");
                        terminal_write_string(buf);
                        terminal_write_string("\n");
                        break;
                    }
                    extern void scheduler_yield(void);
                    scheduler_yield();
                }
            }
        }
    } else if (strcmp(name, "ps") == 0) {
        /* list processes */
        process_list();
    } else if (strcmp(name, "kill") == 0) {
        if (!arg[0]) { terminal_write_string("kill: missing pid\n"); }
        else {
            uint32_t pid = (uint32_t)atoi(arg);
            if (process_kill(pid) == 0) terminal_write_string("killed\n");
            else terminal_write_string("kill: failed\n");
        }
    } else {
        /* Try to run a program from /bin/<cmd> */
        char binpath[64];
        size_t i = 0; binpath[i++] = '/'; binpath[i++] = 'b'; binpath[i++] = 'i'; binpath[i++] = 'n'; binpath[i++] = '/';
        size_t j = 0; while (j < sizeof(binpath) - i - 1 && name[j]) binpath[i + j] = name[j++];
        binpath[i + j] = '\0';
        vfs_node_t *n = vfs_resolve(binpath, cwd);
        if (n && n->type == VFS_FILE) {
            process_t *p = process_exec(binpath, NULL, 1);
            if (!p) terminal_write_string("exec: failed\n");
            else {
                for (;;) {
                    process_t *pp = process_find_by_pid(p->pid);
                    if (pp && pp->state == PROC_TERMINATED) {
                        char buf[32]; itoa(pp->exit_code, buf, 10);
                        terminal_write_string("process exited with code ");
                        terminal_write_string(buf);
                        terminal_write_string("\n");
                        break;
                    }
                    extern void scheduler_yield(void);
                    scheduler_yield();
                }
            }
        } else {
            terminal_write_string("Unknown command: ");
            terminal_write_string(cmd);
            terminal_write_string("\n");
        }
    }
}

void shell_receive_char(char c) {
    if (c == '\n') {
        /* terminate and handle */
        linebuf[linepos] = '\0';
        handle_command(linebuf);
        linepos = 0;
        prompt();
        return;
    }

    if (c == '\b') {
        if (linepos > 0) {
            linepos--;
            terminal_putchar('\b');
        }
        return;
    }

    /* normal char */
    if (linepos < sizeof(linebuf) - 1) {
        linebuf[linepos++] = c;
        terminal_putchar(c);
    }
}
