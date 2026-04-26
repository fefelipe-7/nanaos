#ifndef NANAOS_LOADER_H
#define NANAOS_LOADER_H

#include <stdint.h>
#include <stddef.h>

/* Load ELF from memory buffer. On success returns 0 and fills out_base/out_size/out_entry.
 * The caller owns the memory returned in out_base (allocated with kmalloc). */
int elf_load_from_buffer(const void *data, size_t size, uint8_t **out_base, size_t *out_size, uint64_t *out_entry);

/* Load ELF from VFS path (uses vfs_read_file). */
int elf_load_from_path(const char *path, uint8_t **out_base, size_t *out_size, uint64_t *out_entry);

#endif /* NANAOS_LOADER_H */
