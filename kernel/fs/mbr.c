#include "fs/mbr.h"
#include "log/logger.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Partition entry offsets in MBR */
#define MBR_PARTITION_TABLE 446
#define PART_ENTRY_SIZE 16

static inline uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int mbr_find_fat32_partition(block_device_t *dev, uint64_t *out_lba, uint64_t *out_sectors) {
    if (!dev || !dev->read_sector) return -1;
    uint8_t buf[512];
    if (dev->read_sector(dev->priv, 0, buf) != 0) return -2;

    /* check boot signature */
    if (buf[510] != 0x55 || buf[511] != 0xAA) {
        log_warn("MBR: invalid signature");
        return -3;
    }

    for (int i = 0; i < 4; ++i) {
        const uint8_t *ent = buf + MBR_PARTITION_TABLE + i * PART_ENTRY_SIZE;
        uint8_t type = ent[4];
        uint32_t start = le32(ent + 8);
        uint32_t sectors = le32(ent + 12);
        if (type == 0x0B || type == 0x0C) {
            if (out_lba) *out_lba = start;
            if (out_sectors) *out_sectors = sectors;
            log_info("MBR: FAT32 partition found");
            return 0;
        }
    }
    log_warn("MBR: no FAT32 partition found");
    return -4;
}
