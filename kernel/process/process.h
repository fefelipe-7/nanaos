#ifndef NANAOS_PROCESS_H
#define NANAOS_PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include "memory/vmm.h"

#define PROC_NAME_LEN 32
#define PROC_MAX_FDS 16

typedef struct process_fd {
    int used;
    int fd;
    uint64_t offset;
    char path[128];
} process_fd_t;

typedef enum {
    PROC_RUNNING = 0,
    PROC_READY,
    PROC_BLOCKED,
    PROC_TERMINATED
} proc_state_t;

typedef struct process {
    uint32_t pid;
    char name[PROC_NAME_LEN];
    proc_state_t state;
    uint64_t *stack_ptr; /* saved stack pointer used by context_switch */
    uint8_t *stack_base;
    size_t stack_size;
    int priority;
    struct process *next;
    /* Executable image mapped for this process (if any) */
    uint8_t *image_base;
    size_t image_size;
    int exit_code;
    vmm_address_space_t *address_space;
    uint64_t cr3;
    process_fd_t fds[PROC_MAX_FDS];
} process_t;

/* Create a kernel process that will execute `entry(arg)` when scheduled.
 * Returns the created process or NULL. */
process_t *process_create(const char *name, void (*entry)(void *), void *arg, int priority);

/* Terminate the current process (called by a process to exit). */
void process_exit_current(void);
/* Exit current process with status code */
void process_exit_current_status(int code);

/* Find a process by pid (returns NULL if not found) */
process_t *process_find_by_pid(uint32_t pid);

/* Get the current running process (NULL if in kernel main) */
process_t *process_get_current(void);

/* List processes (print to terminal) */
void process_list(void);

/* Kill a process by pid (force-terminate) — returns 0 on success */
int process_kill(uint32_t pid);

/* Execute an ELF from VFS path; returns the created process or NULL */
process_t *process_exec(const char *path, const char *name, int priority);

#endif /* NANAOS_PROCESS_H */
