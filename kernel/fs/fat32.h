#ifndef NANAOS_FAT32_H
#define NANAOS_FAT32_H

#include <stdint.h>
#include "drivers/block/block.h"

/* Mount FAT32 partition located at partition_lba on device `dev` under mountpoint.`
 * This is read-only and will populate the in-memory VFS under the mountpoint. */
int fat32_mount_partition(block_device_t *dev, uint64_t partition_lba, const char *mountpoint);

#endif /* NANAOS_FAT32_H */
