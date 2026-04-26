#ifndef NANAOS_MBR_H
#define NANAOS_MBR_H

#include <stdint.h>
#include "drivers/block/block.h"

/* Find first FAT32 partition in MBR. Returns 0 on success and fills out_lba/out_sectors. */
int mbr_find_fat32_partition(block_device_t *dev, uint64_t *out_lba, uint64_t *out_sectors);

#endif /* NANAOS_MBR_H */
