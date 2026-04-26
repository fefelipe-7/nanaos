/* simple freestanding userspace program that uses int 0x80 syscalls */
#include <stdint.h>

#define SYS_WRITE 1
#define SYS_EXIT 4

long user_main(void *arg) {
    (void)arg;
    const char msg[] = "hello from ELF loaded from FAT32\n";
    long ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "0"((long)SYS_WRITE), "D"((long)1), "S"((long)msg), "d"((long)(sizeof(msg)-1))
        : "rcx", "r11", "memory");
    (void)ret;
    /* exit with status 0 */
    asm volatile(
        "int $0x80"
        : /* no outputs */
        : "a"((long)SYS_EXIT), "D"((long)0)
        : "rcx", "r11", "memory");
    for (;;) asm volatile("hlt");
    return 0;
}
