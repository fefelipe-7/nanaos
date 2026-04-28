/* simple freestanding userspace program that uses int 0x80 syscalls */
#include <stdint.h>

#define SYS_WRITE 1
#define SYS_GET_TICKS 3
#define SYS_EXIT 4
#define SYS_GETPID 8
#define SYS_SLEEP 9
#define SYS_UPTIME 10

static long syscall3(long n, long a1, long a2, long a3) {
    long ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "0"(n), "D"(a1), "S"(a2), "d"(a3)
                 : "rcx", "r11", "memory");
    return ret;
}

static long syscall1(long n, long a1) {
    return syscall3(n, a1, 0, 0);
}

static long syscall0(long n) {
    return syscall3(n, 0, 0, 0);
}

long user_main(void *arg) {
    (void)arg;
    const char msg[] = "hello from isolated userspace\n";
    (void)syscall3(SYS_WRITE, 1, (long)msg, (long)(sizeof(msg) - 1));
    (void)syscall0(SYS_GETPID);
    (void)syscall0(SYS_GET_TICKS);
    (void)syscall0(SYS_UPTIME);
    (void)syscall1(SYS_SLEEP, 25);
    (void)syscall1(SYS_EXIT, 0);
    for (;;) asm volatile("hlt");
    return 0;
}
