#include "memory/memory_map.h"
#include "terminal/vga.h"
#include "log/logger.h"
#include <stdint.h>

/* Multiboot2 structures (minimal subset used here) */
struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
};

struct mb2_tag {
    uint32_t type;
    uint32_t size;
};

#define MB2_TAG_TYPE_MMAP 6

struct mb2_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    /* entries follow */
};

struct mb2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

static void print_hex64(uint64_t val) {
    char buf[19];
    buf[0] = '0'; buf[1] = 'x'; buf[18] = '\0';
    for (int i = 17; i >= 2; --i) {
        uint8_t nib = val & 0xF;
        buf[i] = nib < 10 ? '0' + nib : 'a' + nib - 10;
        val >>= 4;
    }
    terminal_write_string(buf);
}

void memory_parse_mmap(uint64_t multiboot_info_addr, mmap_entry_cb cb, void *ctx) {
    if (!multiboot_info_addr || !cb) return;

    struct mb2_info *info = (struct mb2_info *)multiboot_info_addr;
    uint8_t *ptr = (uint8_t *)multiboot_info_addr + sizeof(struct mb2_info);
    uint8_t *end = (uint8_t *)multiboot_info_addr + info->total_size;

    while (ptr < end) {
        struct mb2_tag *tag = (struct mb2_tag *)ptr;
        if (tag->type == 0 && tag->size == 8)
            break; /* end tag */

        if (tag->type == MB2_TAG_TYPE_MMAP) {
            struct mb2_tag_mmap *mmap = (struct mb2_tag_mmap *)tag;
            uint8_t *ent = ptr + sizeof(struct mb2_tag_mmap);
            uint8_t *ent_end = ptr + tag->size;
            while (ent + mmap->entry_size <= ent_end) {
                struct mb2_mmap_entry *e = (struct mb2_mmap_entry *)ent;
                cb(e->addr, e->len, e->type, ctx);
                ent += mmap->entry_size;
            }
        }

        /* advance to next tag (8-byte aligned) */
        uint32_t size = tag->size;
        size = (size + 7) & ~7U;
        ptr += size;
    }
}

void memory_print_mmap(uint64_t multiboot_info_addr) {
    if (!multiboot_info_addr) return;

    terminal_write_string("Detected memory map:\n");
    log_info("[INFO] memory map:");

    /* Callback to print entries */
    void print_cb(uint64_t addr, uint64_t len, uint32_t type, void *ctx) {
        (void)ctx;
        terminal_write_string("  base="); print_hex64(addr);
        terminal_write_string(" len="); print_hex64(len);
        terminal_write_string(" type=");
        if (type == 1) terminal_write_string("usable\n"); else terminal_write_string("reserved\n");
    }

    memory_parse_mmap(multiboot_info_addr, print_cb, NULL);
}
