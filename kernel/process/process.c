#include "process/process.h"
#include "scheduler/scheduler.h"
#include "exec/loader.h"
#include "memory/heap.h"
#include "memory/vmm.h"
#include "terminal/vga.h"
#include "lib/string.h"
#include <stdint.h>
#include <stddef.h>

static uint32_t next_pid = 1;

static void process_init_fds(process_t *p) {
    for (int i = 0; i < PROC_MAX_FDS; ++i) {
        p->fds[i].used = 0;
        p->fds[i].fd = i;
        p->fds[i].offset = 0;
        p->fds[i].path[0] = '\0';
    }
    /* stdin/stdout/stderr reservados */
    p->fds[0].used = 1;
    p->fds[1].used = 1;
    p->fds[2].used = 1;
}

process_t *process_create(const char *name, void (*entry)(void *), void *arg, int priority) {
    if (!entry) return NULL;

    process_t *p = (process_t *)kmalloc(sizeof(process_t));
    if (!p) return NULL;

    /* allocate stack (16 KiB) */
    size_t stack_size = 16 * 1024;
    uint8_t *stack = (uint8_t *)kmalloc(stack_size);
    if (!stack) return NULL;

    p->pid = next_pid++;
    /* copy name */
    for (size_t i = 0; i < PROC_NAME_LEN - 1 && name && name[i]; ++i) p->name[i] = name[i];
    p->name[PROC_NAME_LEN - 1] = '\0';
    p->state = PROC_READY;
    p->stack_base = stack;
    p->stack_size = stack_size;
    p->priority = priority;
    p->next = NULL;
    p->image_base = NULL;
    p->image_size = 0;
    p->exit_code = 0;
    p->address_space = vmm_create_address_space();
    p->cr3 = p->address_space ? p->address_space->cr3 : 0;
    process_init_fds(p);

    /* Prepare initial stack frame: layout for context_switch pop sequence:
     * [0] rax, [1] rbx, [2] rcx, [3] rdx, [4] rsi, [5] rdi,
     * [6] rbp, [7] r8, [8] r9, [9] r10, [10] r11, [11] r12,
     * [12] r13, [13] r14, [14] r15, [15] return_rip (entry)
     */
    uintptr_t sp_top = (uintptr_t)stack + stack_size;
    /* align to 16 bytes */
    sp_top &= ~((uintptr_t)0xF);
    uint64_t *sp = (uint64_t *)(sp_top - (16 * sizeof(uint64_t)));

    for (int i = 0; i < 16; ++i) sp[i] = 0;

    sp[5] = (uint64_t)arg; /* rdi = arg */
    sp[15] = (uint64_t)entry; /* ret -> entry */

    p->stack_ptr = sp;

    scheduler_add(p);
    return p;
}

void process_exit_current(void) {
    process_t *cur = process_get_current();
    if (!cur) return;
    cur->state = PROC_TERMINATED;
    /* remove from scheduler queue; scheduler will skip terminated tasks */
    scheduler_remove_current();
    /* yield to next process */
    extern void scheduler_yield(void);
    scheduler_yield();
}

void process_exit_current_status(int code) {
    process_t *cur = process_get_current();
    if (!cur) return;
    cur->exit_code = code;
    cur->state = PROC_TERMINATED;
    scheduler_remove_current();
    extern void scheduler_yield(void);
    scheduler_yield();
}

process_t *process_exec(const char *path, const char *name, int priority) {
    if (!path) return NULL;
    uint8_t *base = NULL; size_t size = 0; uint64_t entry = 0;
    if (elf_load_from_path(path, &base, &size, &entry) != 0) return NULL;

    /* derive name if missing */
    char nm[PROC_NAME_LEN];
    if (name && name[0]) {
        size_t i = 0; for (; i < PROC_NAME_LEN - 1 && name[i]; ++i) nm[i] = name[i]; nm[i] = '\0';
    } else {
        /* simple basename */
        const char *p = path; const char *last = p;
        while (*p) { if (*p == '/') last = p + 1; ++p; }
        size_t i = 0; while (i < PROC_NAME_LEN - 1 && last[i]) { nm[i] = last[i]; ++i; }
        nm[i] = '\0';
    }

    process_t *p = process_create(nm, (void (*)(void *))(uintptr_t)entry, NULL, priority);
    if (!p) {
        return NULL;
    }
    p->image_base = base;
    p->image_size = size;
    p->exit_code = 0;
    return p;
}

/* find by pid: iterate scheduler queue */
process_t *process_find_by_pid(uint32_t pid) {
    return scheduler_find_by_pid(pid);
}

process_t *process_get_current(void) {
    return scheduler_get_current();
}

void process_list(void) {
    scheduler_list();
}

int process_kill(uint32_t pid) {
    return scheduler_kill(pid);
}
