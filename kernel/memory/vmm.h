#ifndef NANAOS_VMM_H
#define NANAOS_VMM_H

#include <stdint.h>

#define VMM_PAGE_PRESENT  (1ULL << 0)
#define VMM_PAGE_WRITABLE (1ULL << 1)
#define VMM_PAGE_USER     (1ULL << 2)

typedef struct vmm_address_space {
    uint64_t *pml4;
    uint64_t cr3;
} vmm_address_space_t;

void vmm_init(void);
int vmm_map_page(vmm_address_space_t *as, uint64_t virt, uint64_t phys, uint64_t flags);
int vmm_unmap_page(vmm_address_space_t *as, uint64_t virt);
vmm_address_space_t *vmm_create_address_space(void);
void vmm_switch_address_space(vmm_address_space_t *as);
vmm_address_space_t *vmm_get_kernel_address_space(void);

#endif
