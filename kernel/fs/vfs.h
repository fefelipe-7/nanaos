#ifndef NANAOS_VFS_H
#define NANAOS_VFS_H

#include <stddef.h>
#include <stdint.h>

typedef enum { VFS_FILE = 1, VFS_DIR = 2 } vfs_type_t;

typedef struct vfs_node vfs_node_t;

/* Initialize the VFS/rAMFS. */
void vfs_init(void);

/* Root node */
vfs_node_t *vfs_get_root(void);

/* Resolve a path relative to `start` (if path starts with '/', resolution
 * begins at the root). Returns NULL if not found. */
vfs_node_t *vfs_resolve(const char *path, vfs_node_t *start);

/* Create a file or directory at `path` (relative to `start` if not absolute).
 * Returns the created node or existing node if already present. */
vfs_node_t *vfs_create(const char *path, int is_dir, vfs_node_t *start);

/* Write contents into a file (create if missing). Returns 0 on success. */
int vfs_write_file(const char *path, const char *data, size_t len, vfs_node_t *start);

/* Read file contents; returns pointer to internal buffer and sets len_out. */
const char *vfs_read_file(const char *path, size_t *len_out, vfs_node_t *start);

/* List directory entries and print them (returns 0 on success). */
int vfs_list_dir(const char *path, vfs_node_t *start);

/* Fill `buf` with an absolute path for `node` (buflen must be large enough). */
void vfs_get_path(vfs_node_t *node, char *buf, size_t buflen);

#endif /* NANAOS_VFS_H */
