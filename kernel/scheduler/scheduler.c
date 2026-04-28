#include "scheduler/scheduler.h"
#include "memory/heap.h"
#include "terminal/vga.h"
#include "log/logger.h"
#include "lib/string.h"
#include "memory/vmm.h"
#include <stdint.h>
#include <stddef.h>

/* Context switch implemented in assembly (saves/restores registers and swaps stacks) */
extern void context_switch(uint64_t **old_sp, uint64_t *new_sp);

static process_t *runqueue_head = NULL;
static process_t *current = NULL;
static uint64_t *kernel_sp = NULL; /* saved kernel stack pointer when switching to process */

static uint64_t tick_count = 0;
static int need_resched = 0;

static process_t *zombie_head = NULL;

void scheduler_init(void) {
    runqueue_head = NULL;
    current = NULL;
    kernel_sp = NULL;
    tick_count = 0;
    need_resched = 0;
    zombie_head = NULL;
}

void scheduler_add(process_t *p) {
    if (!p) return;
    p->next = NULL;
    if (!runqueue_head) {
        runqueue_head = p;
    } else {
        process_t *it = runqueue_head;
        while (it->next) it = it->next;
        it->next = p;
    }
}

void scheduler_remove_current(void) {
    if (!current) return;
    current->state = PROC_TERMINATED;
    /* unlink current from runqueue */
    process_t *prev = NULL;
    process_t *it = runqueue_head;
    while (it) {
        if (it == current) break;
        prev = it; it = it->next;
    }
    if (!it) return;
    if (!prev) runqueue_head = it->next; else prev->next = it->next;
    /* move to zombie list */
    it->next = zombie_head;
    zombie_head = it;
}

/* Keep terminated processes in zombie list for inspection; reaping can be
 * implemented later. */


process_t *scheduler_find_by_pid(uint32_t pid) {
    process_t *it = runqueue_head;
    while (it) {
        if (it->pid == pid) return it;
        it = it->next;
    }
    if (current && current->pid == pid) return current;
    /* Check zombies */
    it = zombie_head;
    while (it) {
        if (it->pid == pid) return it;
        it = it->next;
    }
    return NULL;
}

void scheduler_start(void) {
    if (!runqueue_head) return;
    process_t *next = runqueue_head;
    current = next;
    current->state = PROC_RUNNING;
    /* switch from kernel to process; save kernel rsp in kernel_sp */
    context_switch(&kernel_sp, current->stack_ptr);
}

static process_t *pick_next(void) {
    if (!runqueue_head) return NULL;
    if (!current) return runqueue_head;
    /* round robin: find current in list and return next ready */
    process_t *it = runqueue_head;
    while (it) {
        if (it == current) break;
        it = it->next;
    }
    process_t *cand = it ? it->next : runqueue_head;
    while (cand && cand->state != PROC_READY) {
        cand = cand->next ? cand->next : runqueue_head;
        if (cand == it) return current; /* no other ready */
    }
    if (!cand || cand->state != PROC_READY) return current;
    return cand;
}

void scheduler_yield(void) {
    process_t *next = pick_next();
    if (!next || next == current) return;
    /* clear reschedule request since we're performing a context switch now */
    need_resched = 0;

    if (!current) {
        /* from kernel */
        next->state = PROC_RUNNING;
        current = next;
        if (current->address_space) vmm_switch_address_space(current->address_space);
        context_switch(&kernel_sp, current->stack_ptr);
    } else {
        process_t *prev = current;
        int prev_terminated = (prev->state == PROC_TERMINATED);
        if (!prev_terminated) prev->state = PROC_READY;
        current = next;
        current->state = PROC_RUNNING;
        if (current->address_space) vmm_switch_address_space(current->address_space);
        context_switch(&prev->stack_ptr, current->stack_ptr);

        /* After switching we are running in new context. Terminated processes
         * are left in the zombie list for inspection; no automatic free.
         */
    }
}

void scheduler_tick(void) {
    tick_count++;
    /* simple quantum: every 50 ticks request reschedule */
    if ((tick_count % 50) == 0) need_resched = 1;
}

process_t *scheduler_get_current(void) {
    return current;
}

void scheduler_list(void) {
    terminal_write_string("PID\tSTATE\tNAME\n");
    process_t *it = runqueue_head;
    while (it) {
        char buf[32];
        itoa(it->pid, buf, 10);
        terminal_write_string(buf); terminal_write_string("\t");
        switch (it->state) {
            case PROC_RUNNING: terminal_write_string("RUNNING\t"); break;
            case PROC_READY: terminal_write_string("READY\t"); break;
            case PROC_BLOCKED: terminal_write_string("BLOCKED\t"); break;
            case PROC_TERMINATED: terminal_write_string("TERMINATED\t"); break;
        }
        terminal_write_string(it->name); terminal_write_string("\n");
        it = it->next;
    }
    if (current) {
        terminal_write_string("-- currently running PID: ");
        char buf[32]; itoa(current->pid, buf, 10);
        terminal_write_string(buf); terminal_write_string("\n");
    }
}

int scheduler_kill(uint32_t pid) {
    process_t *p = scheduler_find_by_pid(pid);
    if (!p) return -1;
    if (p == current) {
        p->state = PROC_TERMINATED;
        scheduler_remove_current();
        /* switch to next */
        scheduler_yield();
        return 0;
    }
    /* remove from runqueue */
    process_t *prev = NULL; process_t *it = runqueue_head;
    while (it) {
        if (it == p) break;
        prev = it; it = it->next;
    }
    if (!it) return -1;
    if (!prev) runqueue_head = it->next; else prev->next = it->next;
    it->state = PROC_TERMINATED;
    return 0;
}

int scheduler_needs_reschedule(void) {
    return need_resched;
}
