#ifndef NANAOS_BLOCK_H
#define NANAOS_BLOCK_H

#include <stdint.h>
#include <stddef.h>

typedef struct block_device {
    const char *name;
    uint64_t block_size; /* bytes per sector, usually 512 */
    void *priv;
    /* read one sector (512 bytes) at LBA into buf (must be at least block_size bytes) */
    int (*read_sector)(void *priv, uint64_t lba, void *buf);
} block_device_t;

/* Register a primary block device (e.g. ATA) */
void block_register(block_device_t *dev);
block_device_t *block_get_primary(void);

#endif /* NANAOS_BLOCK_H */
