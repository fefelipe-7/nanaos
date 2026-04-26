#include "fs/fat32.h"
#include "drivers/block/block.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "log/logger.h"
#include "terminal/vga.h"
#include "lib/string.h"
#include <stdint.h>
#include <stddef.h>

/* Minimal FAT32 read-only implementation that loads directory entries and
 * file contents into the RAMFS (vfs) at mount time. Supports basic 8.3 names.
 */

static inline uint16_t le16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static inline uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint32_t fat_size; /* in sectors */
    uint32_t root_cluster;
    uint64_t partition_lba;
    uint64_t first_data_sector;
    size_t sector_size;
    uint8_t *fat; /* full FAT loaded in memory */
} fat32_t;

static int read_sector(block_device_t *dev, uint64_t lba, void *buf) {
    return dev->read_sector(dev->priv, lba, buf);
}

static uint64_t first_sector_of_cluster(fat32_t *fs, uint32_t cluster) {
    return fs->partition_lba + fs->first_data_sector + (uint64_t)(cluster - 2) * fs->sectors_per_cluster;
}

/* Load full FAT into memory */
static int fat_load_fat(fat32_t *fs, block_device_t *dev) {
    size_t fat_bytes = fs->fat_size * fs->bytes_per_sector;
    fs->fat = (uint8_t *)kmalloc(fat_bytes);
    if (!fs->fat) return -1;
    uint8_t *ptr = fs->fat;
    for (uint32_t i = 0; i < fs->fat_size; ++i) {
        if (read_sector(dev, fs->partition_lba + fs->reserved_sectors + i, ptr) != 0) return -2;
        ptr += fs->bytes_per_sector;
    }
    return 0;
}

static uint32_t fat_get_next(fat32_t *fs, uint32_t cluster) {
    uint32_t off = cluster * 4;
    uint8_t *p = fs->fat + off;
    uint32_t val = le32(p) & 0x0FFFFFFF;
    return val;
}

/* Read cluster chain into buffer (allocates with kmalloc). Returns 0 on success. */
static int fat_read_file_clusters(fat32_t *fs, block_device_t *dev, uint32_t start_cluster, uint8_t *out_buf, uint32_t clusters_to_read) {
    uint32_t c = start_cluster;
    size_t sector_bytes = fs->bytes_per_sector;
    uint8_t *dst = out_buf;
    for (uint32_t i = 0; i < clusters_to_read; ++i) {
        uint64_t lba = first_sector_of_cluster(fs, c);
        for (uint32_t s = 0; s < fs->sectors_per_cluster; ++s) {
            if (read_sector(dev, lba + s, dst) != 0) return -2;
            dst += sector_bytes;
        }
        uint32_t next = fat_get_next(fs, c);
        if (next >= 0x0FFFFFF8) break;
        c = next;
    }
    return 0;
}

/* Convert 8.3 name to C-string (uppercase) */
static void decode_83_name(const uint8_t *entry, char *out, size_t outlen) {
    size_t pos = 0;
    for (int i = 0; i < 8 && pos + 2 < outlen; ++i) {
        char c = entry[i];
        if (c == ' ') continue;
        out[pos++] = c;
    }
    /* extension */
    if (entry[8] != ' ') {
        out[pos++] = '.';
        for (int i = 8; i < 11 && pos + 1 < outlen; ++i) {
            char c = entry[i]; if (c == ' ') continue; out[pos++] = c;
        }
    }
    out[pos] = '\0';
}

/* Process directory at cluster and populate VFS under `path` */
static int fat_process_directory(fat32_t *fs, block_device_t *dev, uint32_t cluster, const char *path) {
    size_t cluster_bytes = fs->sectors_per_cluster * fs->bytes_per_sector;
    uint8_t *cluster_buf = (uint8_t *)kmalloc(cluster_bytes);
    if (!cluster_buf) return -1;
    uint32_t c = cluster;
    for (;;) {
        /* read one cluster */
        int r = fat_read_file_clusters(fs, dev, c, cluster_buf, 1);
        if (r != 0) return r;
        /* iterate directory entries */
        for (size_t off = 0; off + 32 <= cluster_bytes; off += 32) {
            uint8_t *ent = cluster_buf + off;
            if (ent[0] == 0x00) { /* end of entries */ break; }
            if (ent[0] == 0xE5) continue; /* deleted */
            uint8_t attr = ent[11];
            if (attr == 0x0F) continue; /* long name */
            char name[32]; decode_83_name(ent, name, sizeof(name));
            uint16_t high = le16(ent + 20);
            uint16_t low = le16(ent + 26);
            uint32_t first_cluster = ((uint32_t)high << 16) | (uint32_t)low;
            uint32_t filesize = le32(ent + 28);

            /* build full path */
            char fullpath[256]; size_t n = 0;
            for (size_t i = 0; i < strlen(path); ++i) fullpath[n++] = path[i];
            if (n > 0 && fullpath[n-1] != '/') fullpath[n++] = '/';
            size_t j = 0; while (j < sizeof(name) && name[j]) fullpath[n++] = name[j++]; fullpath[n] = '\0';

            if (attr & 0x10) {
                /* directory */
                vfs_create(fullpath, 1, vfs_get_root());
                if (first_cluster != 0) {
                    fat_process_directory(fs, dev, first_cluster, fullpath);
                }
            } else {
                /* file: read clusters and create vfs file */
                if (filesize == 0) {
                    vfs_write_file(fullpath, "", 0, vfs_get_root());
                } else {
                    /* compute number of clusters needed */
                    uint32_t sectors_per_cluster = fs->sectors_per_cluster;
                    uint32_t bytes_per_cluster = sectors_per_cluster * fs->bytes_per_sector;
                    uint32_t clusters_needed = (filesize + bytes_per_cluster - 1) / bytes_per_cluster;
                    uint8_t *filebuf = (uint8_t *)kmalloc((size_t)clusters_needed * bytes_per_cluster + 1);
                    if (!filebuf) continue;
                    fat_read_file_clusters(fs, dev, first_cluster, filebuf, clusters_needed);
                    /* write only filesize bytes */
                    vfs_write_file(fullpath, (const char *)filebuf, filesize, vfs_get_root());
                }
            }
        }
        uint32_t next = fat_get_next(fs, c);
        if (next >= 0x0FFFFFF8) break;
        c = next;
    }
    return 0;
}

int fat32_mount_partition(block_device_t *dev, uint64_t partition_lba, const char *mountpoint) {
    if (!dev || !dev->read_sector || !mountpoint) return -1;
    uint8_t buf[512];
    if (read_sector(dev, partition_lba, buf) != 0) return -2;

    /* parse BPB */
    fat32_t fs;
    fs.bytes_per_sector = le16(buf + 11);
    fs.sectors_per_cluster = buf[13];
    fs.reserved_sectors = le16(buf + 14);
    fs.num_fats = buf[16];
    uint16_t fat16sz = le16(buf + 22);
    fs.fat_size = fat16sz ? fat16sz : le32(buf + 36);
    fs.root_cluster = le32(buf + 44);
    fs.partition_lba = partition_lba;
    fs.sector_size = fs.bytes_per_sector;
    fs.first_data_sector = fs.reserved_sectors + (uint64_t)fs.num_fats * fs.fat_size;

    /* basic validation */
    if (fs.bytes_per_sector == 0 || fs.sectors_per_cluster == 0) {
        log_error("FAT32: invalid BPB");
        return -3;
    }

    if (fat_load_fat(&fs, dev) != 0) {
        log_error("FAT32: failed to load FAT");
        return -4;
    }

    /* create mountpoint directory */
    vfs_create(mountpoint, 1, vfs_get_root());

    /* process root directory */
    if (fs.root_cluster == 0) fs.root_cluster = 2;
    if (fat_process_directory(&fs, dev, (uint32_t)fs.root_cluster, mountpoint) != 0) {
        log_error("FAT32: failed to process root dir");
        return -5;
    }

    log_info("FAT32: mounted partition");
    return 0;
}
