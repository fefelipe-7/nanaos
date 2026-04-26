#ifndef NANAOS_PMM_H
#define NANAOS_PMM_H

#include <stdint.h>

/* Initialize the physical memory manager using the Multiboot2 info pointer. */
void pmm_init(uint64_t multiboot_info_addr);

/* Allocate a single 4 KiB physical frame. Returns physical address, or 0 on failure. */
uint64_t pmm_alloc_frame(void);

/* Free a single 4 KiB physical frame previously allocated. */
void pmm_free_frame(uint64_t phys_addr);

/* Allocate contiguous frames (count). Returns base physical address or 0. */
uint64_t pmm_alloc_frames(uint64_t count);

/* Free contiguous frames starting at phys_addr (count frames). */
void pmm_free_frames(uint64_t phys_addr, uint64_t count);

/* Statistics */
uint64_t pmm_total_memory(void);
uint64_t pmm_used_memory(void);
uint64_t pmm_free_memory(void);

#endif /* NANAOS_PMM_H */
