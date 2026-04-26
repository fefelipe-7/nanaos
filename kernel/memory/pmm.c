#include "memory/pmm.h"
#include "memory/memory_map.h"
#include "log/logger.h"
#include "terminal/vga.h"
#include "lib/string.h"
#include "memory/heap.h"
#include <stdint.h>

/* Kernel symbols provided by linker.ld */
extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

#define PAGE_SIZE 4096ULL
#define MAX_FREE_REGIONS 64

struct free_region {
    uint64_t addr;    /* physical start address, aligned to PAGE_SIZE */
    uint64_t frames;  /* number of 4KiB frames */
};

static struct free_region free_regions[MAX_FREE_REGIONS];
static size_t free_count = 0;

static uint64_t total_memory = 0;

static inline uint64_t align_up(uint64_t a, uint64_t align) {
    return (a + align - 1) & ~(align - 1);
}

static inline uint64_t align_down(uint64_t a, uint64_t align) {
    return a & ~(align - 1);
}

static void add_free_region(uint64_t addr, uint64_t len) {
    if (len < PAGE_SIZE) return;

    uint64_t start = align_up(addr, PAGE_SIZE);
    uint64_t end = align_down(addr + len, PAGE_SIZE);
    if (end <= start) return;

    uint64_t frames = (end - start) / PAGE_SIZE;
    if (frames == 0) return;

    /* Try to merge with existing regions (simple coalesce) */
    for (size_t i = 0; i < free_count; ++i) {
        uint64_t rstart = free_regions[i].addr;
        uint64_t rend = rstart + free_regions[i].frames * PAGE_SIZE;
        if (end == rstart) {
            /* extend region backward */
            free_regions[i].addr = start;
            free_regions[i].frames += frames;
            return;
        } else if (start == rend) {
            /* extend region forward */
            free_regions[i].frames += frames;
            return;
        }
    }

    if (free_count < MAX_FREE_REGIONS) {
        free_regions[free_count].addr = start;
        free_regions[free_count].frames = frames;
        free_count++;
    }
}

static void mmap_callback(uint64_t addr, uint64_t len, uint32_t type, void *ctx) {
    (void)ctx;
    /* Only type==1 is usable RAM */
    if (type != 1) return;

    /* Don't include low memory below 1 MiB (BIOS/MBR area) */
    uint64_t region_start = addr;
    uint64_t region_len = len;
    if (region_start + region_len <= 0x100000ULL) return;
    if (region_start < 0x100000ULL) {
        uint64_t diff = 0x100000ULL - region_start;
        region_start += diff;
        region_len -= diff;
    }

    /* Exclude kernel physical footprint and the reserved kernel heap after it */
    uint64_t kstart = (uint64_t)&__kernel_start;
    uint64_t kend = (uint64_t)&__kernel_end + (uint64_t)KERNEL_HEAP_SIZE;

    uint64_t rstart = region_start;
    uint64_t rend = region_start + region_len;

    if (rend <= kstart || rstart >= kend) {
        /* no overlap */
        add_free_region(rstart, rend - rstart);
    } else {
        /* overlap: add lower part if any */
        if (rstart < kstart) {
            add_free_region(rstart, kstart - rstart);
        }
        /* add upper part if any */
        if (rend > kend) {
            add_free_region(kend, rend - kend);
        }
    }

    total_memory += len;
}

void pmm_init(uint64_t multiboot_info_addr) {
    /* Reset state */
    free_count = 0;
    total_memory = 0;

    log_info("[INFO] parsing multiboot memory map");
    memory_parse_mmap(multiboot_info_addr, mmap_callback, NULL);

    /* Report totals */
    char buf[32];
    itoa(total_memory / 1024, buf, 10);
    terminal_write_string("Total memory (KB): ");
    terminal_write_string(buf);
    terminal_write_string("\n");
    log_info("[OK] pmm initialized");
}

uint64_t pmm_alloc_frame(void) {
    for (size_t i = 0; i < free_count; ++i) {
        if (free_regions[i].frames == 0) continue;
        uint64_t addr = free_regions[i].addr;
        free_regions[i].addr += PAGE_SIZE;
        free_regions[i].frames -= 1;
        if (free_regions[i].frames == 0) {
            /* compact array */
            for (size_t j = i + 1; j < free_count; ++j)
                free_regions[j - 1] = free_regions[j];
            free_count--;
        }
        return addr;
    }
    return 0; /* out of memory */
}

void pmm_free_frame(uint64_t phys_addr) {
    /* Coalesce: try to merge with neighboring regions */
    if (phys_addr % PAGE_SIZE != 0) return; /* invalid */

    for (size_t i = 0; i < free_count; ++i) {
        uint64_t rstart = free_regions[i].addr;
        uint64_t rend = rstart + free_regions[i].frames * PAGE_SIZE;
        if (phys_addr + PAGE_SIZE == rstart) {
            free_regions[i].addr = phys_addr;
            free_regions[i].frames += 1;
            return;
        } else if (phys_addr == rend) {
            free_regions[i].frames += 1;
            return;
        }
    }

    if (free_count < MAX_FREE_REGIONS) {
        free_regions[free_count].addr = phys_addr;
        free_regions[free_count].frames = 1;
        free_count++;
    }
}

uint64_t pmm_alloc_frames(uint64_t count) {
    if (count == 0) return 0;
    for (size_t i = 0; i < free_count; ++i) {
        if (free_regions[i].frames >= count) {
            uint64_t addr = free_regions[i].addr;
            free_regions[i].addr += count * PAGE_SIZE;
            free_regions[i].frames -= count;
            if (free_regions[i].frames == 0) {
                for (size_t j = i + 1; j < free_count; ++j)
                    free_regions[j - 1] = free_regions[j];
                free_count--;
            }
            return addr;
        }
    }
    return 0;
}

void pmm_free_frames(uint64_t phys_addr, uint64_t count) {
    if (count == 0) return;
    /* Free frames one contiguous block */
    if (phys_addr % PAGE_SIZE != 0) return;
    add_free_region(phys_addr, count * PAGE_SIZE);
}

uint64_t pmm_total_memory(void) {
    return total_memory;
}

uint64_t pmm_free_memory(void) {
    uint64_t free = 0;
    for (size_t i = 0; i < free_count; ++i) {
        free += free_regions[i].frames * PAGE_SIZE;
    }
    return free;
}

uint64_t pmm_used_memory(void) {
    return total_memory - pmm_free_memory();
}
