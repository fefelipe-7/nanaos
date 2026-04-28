#include "drivers/ata/ata.h"
#include "arch/x86_64/port/port.h"
#include "drivers/block/block.h"
#include "log/logger.h"
#include "terminal/vga.h"
#include <stdint.h>
#include <stddef.h>

/* Primary ATA ports */
#define ATA_PRIMARY_BASE 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

#define ATA_REG_DATA     0x00
#define ATA_REG_ERROR    0x01
#define ATA_REG_SECCNT   0x02
#define ATA_REG_LBA0     0x03
#define ATA_REG_LBA1     0x04
#define ATA_REG_LBA2     0x05
#define ATA_REG_DEV      0x06
#define ATA_REG_STATUS   0x07
#define ATA_REG_COMMAND  0x07

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_SR_BSY  0x80
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

static block_device_t ata_dev;

static int ata_block_read_sector(void *priv, uint64_t lba, void *buf) {
    (void)priv;
    return ata_read_sector(lba, buf);
}

static inline uint16_t inw_port(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

static inline void outw_port(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a" (val), "dN" (port));
}

static void ata_delay(void) {
    /* 400ns delay: read alt status 4 times */
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
}

static int ata_poll_status(void) {
    uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    while (status & ATA_SR_BSY) {
        ata_delay();
        status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    }
    return status;
}

int ata_init(void) {
    /* Try to identify drive 0 (master) */
    outb(ATA_PRIMARY_BASE + ATA_REG_DEV, 0xA0); /* select master */
    ata_delay();
    outb(ATA_PRIMARY_BASE + ATA_REG_SECCNT, 0);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA0, 0);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA1, 0);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA2, 0);
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if (status == 0) {
        log_warn("ATA: no device (status=0)");
        return -1;
    }
    /* poll until DRQ or ERR */
    while (!(status & (ATA_SR_DRQ | ATA_SR_ERR))) {
        status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            log_error("ATA: identify failed (ERR)");
            return -1;
        }
    }

    /* read IDENTIFY (256 words) just to clear data; we don't parse it now */
    for (int i = 0; i < 256; ++i) {
        (void)inw_port(ATA_PRIMARY_BASE + ATA_REG_DATA);
    }

    /* Register block device */
    ata_dev.name = "ata0";
    ata_dev.block_size = 512;
    ata_dev.priv = NULL;
    ata_dev.read_sector = ata_block_read_sector;
    block_register(&ata_dev);

    log_info("ATA: primary initialized");
    return 0;
}

block_device_t *ata_get_blockdev(void) {
    return &ata_dev;
}

int ata_read_sector(uint64_t lba, void *buf) {
    if (!buf) return -1;
    /* only support LBA28 for now */
    if (lba > 0x0FFFFFFF) return -2;

    /* Select drive and head with high 4 bits of LBA */
    outb(ATA_PRIMARY_BASE + ATA_REG_DEV, 0xE0 | (uint8_t)((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_BASE + ATA_REG_SECCNT, 1);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    /* Poll for readiness */
    uint8_t status = ata_poll_status();
    if (status & ATA_SR_ERR) return -3;
    /* wait for DRQ */
    while (!(inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_DRQ));

    /* read 256 words = 512 bytes */
    uint16_t *w = (uint16_t *)buf;
    for (int i = 0; i < 256; ++i) {
        w[i] = inw_port(ATA_PRIMARY_BASE + ATA_REG_DATA);
    }

    return 0;
}

int ata_selftest_sector0(void) {
    uint8_t buf[512];
    int rc = ata_read_sector(0, buf);
    if (rc != 0) {
        log_error("ATA: sector0 read failed");
        return rc;
    }
    log_info("ATA: sector0 read ok");
    return 0;
}
