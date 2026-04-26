#ifndef NANAOS_MEMORY_MAP_H
#define NANAOS_MEMORY_MAP_H

#include <stdint.h>

/* Callback invoked for each memory map entry parsed from Multiboot2. */
typedef void (*mmap_entry_cb)(uint64_t addr, uint64_t len, uint32_t type, void *ctx);

/* Parse Multiboot2 memory map tag and call `cb` for each entry. */
void memory_parse_mmap(uint64_t multiboot_info_addr, mmap_entry_cb cb, void *ctx);

/* Pretty-print memory map to terminal and serial. */
void memory_print_mmap(uint64_t multiboot_info_addr);

#endif /* NANAOS_MEMORY_MAP_H */
