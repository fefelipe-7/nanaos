#ifndef NANAOS_SCHEDULER_H
#define NANAOS_SCHEDULER_H

#include <stdint.h>
#include "process/process.h"

/* Initialize scheduler data structures */
void scheduler_init(void);

/* Add a process to the runqueue */
void scheduler_add(process_t *p);

/* Remove current process from runqueue (used on exit) */
void scheduler_remove_current(void);

/* Find process by pid */
process_t *scheduler_find_by_pid(uint32_t pid);

/* Start the scheduler (no-op for cooperative mode) */
void scheduler_start(void);

/* Yield CPU to next runnable process */
void scheduler_yield(void);

/* Called periodically by timer IRQ to update ticks and request reschedule */
void scheduler_tick(void);

/* Get the current process */
process_t *scheduler_get_current(void);

/* Print the runqueue */
void scheduler_list(void);

/* Kill a process by pid (force termination) */
int scheduler_kill(uint32_t pid);

/* Query whether scheduler requested a reschedule (set by tick logic). */
int scheduler_needs_reschedule(void);

#endif /* NANAOS_SCHEDULER_H */
