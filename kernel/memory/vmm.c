#include "memory/vmm.h"
#include "memory/heap.h"
#include "log/logger.h"
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096ULL
#define PAGE_MASK (~(PAGE_SIZE - 1ULL))
#define ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ULL

static vmm_address_space_t kernel_as;
static vmm_address_space_t *current_as = NULL;

static inline uint64_t read_cr3(void) {
    uint64_t v;
    __asm__ volatile("mov %%cr3, %0" : "=r"(v));
    return v;
}

static inline void write_cr3(uint64_t v) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(v) : "memory");
}

static uint64_t *vmm_alloc_table(void) {
    uint64_t *t = (uint64_t *)kmalloc(PAGE_SIZE);
    if (!t) return NULL;
    for (size_t i = 0; i < PAGE_SIZE / sizeof(uint64_t); ++i) t[i] = 0;
    return t;
}

void vmm_init(void) {
    uint64_t cr3 = read_cr3() & ENTRY_ADDR_MASK;
    kernel_as.pml4 = (uint64_t *)(uintptr_t)cr3;
    kernel_as.cr3 = cr3;
    current_as = &kernel_as;
    log_info("[OK] vmm ready");
}

static uint64_t *vmm_next_table(uint64_t *table, uint16_t idx, uint64_t flags, int create) {
    uint64_t e = table[idx];
    if (e & VMM_PAGE_PRESENT) {
        return (uint64_t *)(uintptr_t)(e & ENTRY_ADDR_MASK);
    }
    if (!create) return NULL;
    uint64_t *new_table = vmm_alloc_table();
    if (!new_table) return NULL;
    table[idx] = ((uint64_t)(uintptr_t)new_table & ENTRY_ADDR_MASK) | (flags & 0xFFFULL) | VMM_PAGE_PRESENT;
    return new_table;
}

int vmm_map_page(vmm_address_space_t *as, uint64_t virt, uint64_t phys, uint64_t flags) {
    if (!as || !as->pml4) return -1;
    uint16_t i4 = (virt >> 39) & 0x1FF;
    uint16_t i3 = (virt >> 30) & 0x1FF;
    uint16_t i2 = (virt >> 21) & 0x1FF;
    uint16_t i1 = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = vmm_next_table(as->pml4, i4, flags, 1);
    if (!pdpt) return -2;
    uint64_t *pd = vmm_next_table(pdpt, i3, flags, 1);
    if (!pd) return -3;
    uint64_t *pt = vmm_next_table(pd, i2, flags, 1);
    if (!pt) return -4;
    pt[i1] = (phys & ENTRY_ADDR_MASK) | (flags & 0xFFFULL) | VMM_PAGE_PRESENT;
    return 0;
}

int vmm_unmap_page(vmm_address_space_t *as, uint64_t virt) {
    if (!as || !as->pml4) return -1;
    uint16_t i4 = (virt >> 39) & 0x1FF;
    uint16_t i3 = (virt >> 30) & 0x1FF;
    uint16_t i2 = (virt >> 21) & 0x1FF;
    uint16_t i1 = (virt >> 12) & 0x1FF;
    uint64_t *pdpt = vmm_next_table(as->pml4, i4, 0, 0);
    if (!pdpt) return -2;
    uint64_t *pd = vmm_next_table(pdpt, i3, 0, 0);
    if (!pd) return -3;
    uint64_t *pt = vmm_next_table(pd, i2, 0, 0);
    if (!pt) return -4;
    pt[i1] = 0;
    return 0;
}

vmm_address_space_t *vmm_create_address_space(void) {
    vmm_address_space_t *as = (vmm_address_space_t *)kmalloc(sizeof(vmm_address_space_t));
    if (!as) return NULL;
    as->pml4 = vmm_alloc_table();
    if (!as->pml4) return NULL;

    if (kernel_as.pml4) {
        for (size_t i = 0; i < PAGE_SIZE / sizeof(uint64_t); ++i) {
            as->pml4[i] = kernel_as.pml4[i];
        }
    }
    as->cr3 = (uint64_t)(uintptr_t)as->pml4 & ENTRY_ADDR_MASK;
    return as;
}

void vmm_switch_address_space(vmm_address_space_t *as) {
    if (!as || !as->pml4) return;
    if (current_as == as) return;
    current_as = as;
    write_cr3(as->cr3);
}

vmm_address_space_t *vmm_get_kernel_address_space(void) {
    return &kernel_as;
}
