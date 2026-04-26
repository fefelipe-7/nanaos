#ifndef NANAOS_SYSCALL_H
#define NANAOS_SYSCALL_H

#include <stdint.h>

/* Syscall numbers */
#define SYS_WRITE      1
#define SYS_READ       2
#define SYS_GET_TICKS  3
#define SYS_EXIT       4
#define SYS_OPEN       5
#define SYS_CLOSE      6

/* Dispatcher: returns uint64_t value in RAX */
uint64_t syscall_dispatch(uint64_t num,
                          uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5, uint64_t a6);

#endif /* NANAOS_SYSCALL_H */
