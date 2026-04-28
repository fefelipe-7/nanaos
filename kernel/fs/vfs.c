#include "fs/vfs.h"
#include "memory/heap.h"
#include "lib/string.h"
#include "terminal/vga.h"
#include "log/logger.h"
#include <stdint.h>
#include <stddef.h>
/* Embedded userspace binaries (generated at build time) */
#include "exec/hello_elf.h"

struct vfs_node {
    char *name;
    int type; /* VFS_FILE or VFS_DIR */
    struct vfs_node *parent;
    struct vfs_node *children; /* head of singly-linked list */
    struct vfs_node *next;     /* sibling link */

    /* file data */
    char *data;
    size_t size;
};

static struct vfs_node *vfs_root = NULL;

/* helpers */
static char *vfs_strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = (char *)kmalloc(n);
    if (!d) return NULL;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return d;
}

static struct vfs_node *vfs_node_new(const char *name, int type) {
    struct vfs_node *n = (struct vfs_node *)kmalloc(sizeof(struct vfs_node));
    if (!n) return NULL;
    n->name = vfs_strdup(name ? name : "");
    n->type = type;
    n->parent = NULL;
    n->children = NULL;
    n->next = NULL;
    n->data = NULL;
    n->size = 0;
    return n;
}

static struct vfs_node *vfs_find_child(struct vfs_node *dir, const char *name) {
    if (!dir || dir->type != VFS_DIR) return NULL;
    for (struct vfs_node *c = dir->children; c; c = c->next) {
        if (strcmp(c->name, name) == 0) return c;
    }
    return NULL;
}

/* resolve tokens of path (supports '.', '..', absolute and relative paths) */
static struct vfs_node *vfs_resolve_tokens(struct vfs_node *start, const char **tokens, size_t count) {
    struct vfs_node *cur = start;
    for (size_t i = 0; i < count; ++i) {
        const char *tok = tokens[i];
        if (tok[0] == '\0' || strcmp(tok, ".") == 0) continue;
        if (strcmp(tok, "..") == 0) {
            if (cur->parent) cur = cur->parent;
            continue;
        }
        struct vfs_node *child = vfs_find_child(cur, tok);
        if (!child) return NULL;
        cur = child;
    }
    return cur;
}

vfs_node_t *vfs_resolve(const char *path, vfs_node_t *start) {
    if (!path) return NULL;
    if (!start) start = vfs_root;

    /* special-case root */
    if (path[0] == '/' && (path[1] == '\0' || path[1] == '\n')) return vfs_root;

    /* tokenize path into small stack array */
    const char *p = path;
    const size_t MAX_TOKENS = 64;
    const char *tokens[MAX_TOKENS];
    size_t tcount = 0;

    if (path[0] == '/') {
        start = vfs_root;
        p = path + 1;
    }

    while (*p && tcount < MAX_TOKENS) {
        /* skip slashes */
        while (*p == '/') ++p;
        if (!*p) break;
        /* begin token */
        const char *q = p;
        while (*q && *q != '/') ++q;
        size_t len = q - p;
        char *buf = (char *)kmalloc(len + 1);
        if (!buf) return NULL;
        for (size_t i = 0; i < len; ++i) buf[i] = p[i];
        buf[len] = '\0';
        tokens[tcount++] = buf;
        p = q;
    }

    struct vfs_node *res = vfs_resolve_tokens(start, tokens, tcount);

    /* cleanup token buffers is not necessary since kmalloc is bump and kfree noop,
     * but we avoid trying to free them. */
    return res;
}

vfs_node_t *vfs_get_root(void) {
    return vfs_root;
}

vfs_node_t *vfs_create(const char *path, int is_dir, vfs_node_t *start) {
    if (!path) return NULL;
    if (!start) start = vfs_root;

    /* Strip trailing slashes */
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/') --len;

    /* find parent path and name */
    size_t pos = len;
    while (pos > 0 && path[pos - 1] != '/') --pos;
    const char *name = path + pos;
    char parent_buf[256];
    if (pos == 0) {
        /* parent is start (or root for absolute) */
        if (path[0] == '/') {
            parent_buf[0] = '/'; parent_buf[1] = '\0';
        } else {
            parent_buf[0] = '.'; parent_buf[1] = '\0';
        }
    } else {
        size_t plen = pos;
        if (plen >= sizeof(parent_buf)) plen = sizeof(parent_buf) - 1;
        for (size_t i = 0; i < plen; ++i) parent_buf[i] = path[i];
        parent_buf[plen] = '\0';
    }

    struct vfs_node *parent = vfs_resolve(parent_buf, start);
    if (!parent || parent->type != VFS_DIR) return NULL;

    /* If exists return it */
    struct vfs_node *existing = vfs_find_child(parent, name);
    if (existing) return existing;

    struct vfs_node *node = vfs_node_new(name, is_dir ? VFS_DIR : VFS_FILE);
    if (!node) return NULL;
    node->parent = parent;
    /* insert at head */
    node->next = parent->children;
    parent->children = node;
    return node;
}

int vfs_write_file(const char *path, const char *data, size_t len, vfs_node_t *start) {
    if (!path) return -1;
    struct vfs_node *n = vfs_create(path, 0, start);
    if (!n) return -1;
    if (n->type != VFS_FILE) return -1;
    /* allocate data buffer */
    char *buf = (char *)kmalloc(len + 1);
    if (!buf) return -1;
    for (size_t i = 0; i < len; ++i) buf[i] = data[i];
    buf[len] = '\0';
    n->data = buf;
    n->size = len;
    return 0;
}

const char *vfs_read_file(const char *path, size_t *len_out, vfs_node_t *start) {
    struct vfs_node *n = vfs_resolve(path, start);
    if (!n || n->type != VFS_FILE) return NULL;
    if (len_out) *len_out = n->size;
    return n->data;
}

int vfs_list_dir(const char *path, vfs_node_t *start) {
    struct vfs_node *n = vfs_resolve(path, start);
    if (!n) return -1;
    if (n->type != VFS_DIR) return -1;
    for (struct vfs_node *c = n->children; c; c = c->next) {
        terminal_write_string(c->name);
        terminal_write_string(" ");
    }
    terminal_write_string("\n");
    return 0;
}

void vfs_get_path(vfs_node_t *node, char *buf, size_t buflen) {
    if (!node || !buf || buflen == 0) return;
    /* Walk up to root collecting names (simple reversed stack) */
    const char *parts[64];
    size_t pcount = 0;
    struct vfs_node *cur = node;
    while (cur && cur != vfs_root && pcount < 64) {
        parts[pcount++] = cur->name;
        cur = cur->parent;
    }
    /* root */
    size_t off = 0;
    if (node == vfs_root) {
        if (buflen >= 2) {
            buf[0] = '/'; buf[1] = '\0';
        } else if (buflen >= 1) {
            buf[0] = '\0';
        }
        return;
    }
    /* build path from parts in reverse */
    for (size_t i = 0; i < pcount; ++i) {
        const char *name = parts[pcount - 1 - i];
        if (off + 1 >= buflen) break;
        buf[off++] = '/';
        size_t j = 0;
        while (name[j] && off + 1 < buflen) buf[off++] = name[j++];
    }
    buf[off] = '\0';
}

void vfs_init(void) {
    if (vfs_root) return;
    vfs_root = vfs_node_new("", VFS_DIR);
    vfs_root->parent = NULL;

    /* Create /system and /home */
    vfs_create("/system", 1, vfs_root);
    vfs_create("/home", 1, vfs_root);

    /* Create initial files */
    vfs_write_file("/system/version", "nanaos v0.1\n", strlen("nanaos v0.1\n"), vfs_root);
    vfs_write_file("/system/about", "nanaos experimental operating system\n", strlen("nanaos experimental operating system\n"), vfs_root);
    vfs_write_file("/home/readme.txt", "Welcome to nanaos!\n", strlen("Welcome to nanaos!\n"), vfs_root);

    /* If an embedded /bin/hello exists (generated by build), register it */
    if ((size_t)(&userspace_hello_hello_elf_len) != 0) {
        /* symbols are generated by xxd from userspace/hello/hello.elf */
        if ((size_t)userspace_hello_hello_elf_len > 0) {
            vfs_create("/bin", 1, vfs_root);
            vfs_write_file("/bin/hello",
                           (const char *)userspace_hello_hello_elf,
                           (size_t)userspace_hello_hello_elf_len,
                           vfs_root);
        }
    }

    log_info("[OK] vfs initialized");
}
