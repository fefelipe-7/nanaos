#ifndef NANAOS_ATA_H
#define NANAOS_ATA_H

#include <stdint.h>
#include "drivers/block/block.h"

/* Initialize ATA primary channel (returns 0 on success) */
int ata_init(void);

/* Read one 512-byte sector into buf */
int ata_read_sector(uint64_t lba, void *buf);

/* Get block device for the ATA primary device (NULL if none) */
block_device_t *ata_get_blockdev(void);

#endif /* NANAOS_ATA_H */
