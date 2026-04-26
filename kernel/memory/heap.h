#ifndef NANAOS_HEAP_H
#define NANAOS_HEAP_H

#include <stddef.h>

/* Size reserved for the kernel heap (placed right after the kernel in memory).
 * Must remain small enough to fit in the identity mapped area created by
 * the bootstrap (first 8 MiB). Adjust if you change identity mappings. */
#define KERNEL_HEAP_SIZE (1024 * 1024) /* 1 MiB */

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif /* NANAOS_HEAP_H */
