#include "memory/heap.h"
#include <stdint.h>
#include <stddef.h>

/* Simple bump allocator placed immediately after the kernel image.
 * This avoids allocating physical frames that may not be identity-mapped
 * by the early page tables. kfree() is a no-op (placeholder safe).
 */

extern uint8_t __kernel_end; /* provided by linker.ld */

static uint8_t *heap_base = NULL;
static uint8_t *heap_ptr = NULL;
static uint8_t *heap_limit = NULL;

static void heap_ensure_init(void) {
    if (heap_ptr) return;
    heap_base = (uint8_t *)&__kernel_end;
    heap_ptr = heap_base;
    /* avoid static analyzer array-bounds on linker symbol by computing via uintptr_t */
    uintptr_t base = (uintptr_t)heap_base;
    heap_limit = (uint8_t *)(base + (uintptr_t)KERNEL_HEAP_SIZE);
}

static inline size_t align_up_size(size_t v, size_t a) {
    return (v + (a - 1)) & ~(a - 1);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    heap_ensure_init();
    size = align_up_size(size, 8);
    if ((uint8_t *)heap_ptr + size > (uint8_t *)heap_limit) {
        return NULL; /* out of heap */
    }
    void *p = (void *)heap_ptr;
    heap_ptr += size;
    return p;
}

void kfree(void *ptr) {
    (void)ptr; /* noop for now */
}
