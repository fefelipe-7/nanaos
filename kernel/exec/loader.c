#include "exec/loader.h"
#include "exec/elf.h"
#include "memory/heap.h"
#include "fs/vfs.h"
#include "log/logger.h"
#include "terminal/vga.h"
#include <stdint.h>
#include <stddef.h>

static inline size_t align_up(size_t v, size_t a) { return (v + a - 1) & ~(a - 1); }

int elf_load_from_buffer(const void *data, size_t size, uint8_t **out_base, size_t *out_size, uint64_t *out_entry) {
    if (!data || size == 0 || !out_base || !out_size || !out_entry) return -1;
    Elf64_Ehdr ehdr;
    int r = elf_validate(data, size, &ehdr);
    if (r != 0) {
        log_error("ELF: validation failed");
        return -2;
    }

    /* find PT_LOAD segments bounds */
    uint64_t min_vaddr = (uint64_t)-1;
    uint64_t max_vaddr = 0;
    for (unsigned i = 0; i < ehdr.e_phnum; ++i) {
        Elf64_Phdr ph;
        if (elf_read_phdr(data, size, &ehdr, i, &ph) != 0) return -3;
        if (ph.p_type != PT_LOAD) continue;
        if (ph.p_vaddr < min_vaddr) min_vaddr = ph.p_vaddr;
        uint64_t seg_end = ph.p_vaddr + ph.p_memsz;
        if (seg_end > max_vaddr) max_vaddr = seg_end;
    }
    if (min_vaddr == (uint64_t)-1 || max_vaddr == 0) {
        log_error("ELF: no loadable segments");
        return -4;
    }

    size_t total = (size_t)(max_vaddr - min_vaddr);
    total = align_up(total, 0x1000);

    uint8_t *mem = (uint8_t *)kmalloc(total);
    if (!mem) return -5;
    /* zero memory */
    for (size_t i = 0; i < total; ++i) mem[i] = 0;

    /* copy segments */
    for (unsigned i = 0; i < ehdr.e_phnum; ++i) {
        Elf64_Phdr ph;
        if (elf_read_phdr(data, size, &ehdr, i, &ph) != 0) return -6;
        if (ph.p_type != PT_LOAD) continue;
        if (ph.p_offset + ph.p_filesz > size) return -7;
        uint8_t *dest = mem + (size_t)(ph.p_vaddr - min_vaddr);
        const uint8_t *src = (const uint8_t *)data + (size_t)ph.p_offset;
        for (size_t n = 0; n < (size_t)ph.p_filesz; ++n) dest[n] = src[n];
        /* remaining bytes (bss) already zeroed */
    }

    *out_base = mem;
    *out_size = total;
    *out_entry = (uint64_t)(mem + (size_t)(ehdr.e_entry - min_vaddr));
    return 0;
}

int elf_load_from_path(const char *path, uint8_t **out_base, size_t *out_size, uint64_t *out_entry) {
    if (!path) return -1;
    size_t len = 0;
    const char *data = vfs_read_file(path, &len, vfs_get_root());
    if (!data) return -2;
    return elf_load_from_buffer((const void *)data, len, out_base, out_size, out_entry);
}
