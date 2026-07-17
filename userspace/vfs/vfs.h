#pragma once

#include <uapi/uapi.h>
#include <userspace/libc/list.h>

#define VFS_SYMLINK_MAX 8u   /* Nested symlink resolution limit */
#define VFS_CLIENT_MAX  9u   /* Badge-indexed client table size */
#define VFS_FD_MAX      32u  /* Descriptors per client          */
#define DENTRY_INLINE   32u  /* Inline dentry name bytes        */
#define DENTRY_MOUNTED  0x1u /* Dentry is a mountpoint          */
#define DENTRY_ONLRU    0x2u /* Dentry is on the LRU list       */

typedef struct qstr_s   qstr_s;
typedef struct dentry_s dentry_s;
typedef struct rnode_s  rnode_s;
typedef struct rsb_s    rsb_s;
typedef struct fstype_s fstype_s;
typedef struct file_s   file_s;
typedef struct mount_s  mount_s;
typedef struct path_s   path_s;

struct qstr_s {
    const char* name; /* Component bytes  */
    u32         len;  /* Component length */
    u32         hash; /* Full name hash   */
};

struct path_s {
    mount_s*  mount;  /* Mount the dentry was reached through */
    dentry_s* dentry; /* Dentry within the mount              */
};

struct fstype_s {
    const char* name; /* Filesystem name                    */
    u32         ep;   /* Filesystem server endpoint pointer */
};

struct rsb_s {
    u32             count;     /* Reference count          */
    u32             dev;       /* Local device identifier  */
    u32             sb;        /* Remote superblock handle */
    u32             root_ino;  /* Remote root node id      */
    u32             blocksize; /* Block size in bytes      */
    u32             magic;     /* Filesystem magic number  */
    const fstype_s* fstype;    /* Serving filesystem type  */
};

struct rnode_s {
    u32          count;          /* Local reference count       */
    u32          remote;         /* Remote references to forget */
    u32          ino;            /* Remote node id              */
    u32          mode;           /* Type and mode bits          */
    rsb_s*       sb;             /* Owning remote superblock    */
    hlist_node_s hash_list_node; /* Node hash chain link        */
};

struct dentry_s {
    u32          count;                /* Reference count                */
    u32          flags;                /* DENTRY_* flags                 */
    dentry_s*    parent;               /* Parent dentry, self at root    */
    rnode_s*     node;                 /* Bound node, NULL when negative */
    rsb_s*       sb;                   /* Owning remote superblock       */
    qstr_s       name;                 /* Component name                 */
    char         iname[DENTRY_INLINE]; /* Inline short-name storage      */
    hlist_node_s hash_list_node;       /* Dentry hash chain link         */
    list_node_s  child_list_node;      /* Sibling link in parent         */
    list_node_s  children;             /* Child dentries                 */
    list_node_s  lru_list_node;        /* LRU link when unreferenced     */
};

struct mount_s {
    u32          count;           /* Reference count               */
    mount_s*     parent;          /* Parent mount, NULL at root    */
    dentry_s*    mountpoint;      /* Covered dentry in the parent  */
    dentry_s*    root;            /* Root dentry of this mount     */
    rsb_s*       sb;              /* Mounted remote superblock     */
    hlist_node_s hash_list_node;  /* Mount hash chain link         */
    list_node_s  child_list_node; /* Sibling link in parent        */
    list_node_s  children;        /* Mounts on top of this mount   */
    list_node_s  mount_list_node; /* Link in the global mount list */
};

struct file_s {
    u32    count; /* Reference count         */
    u32    flags; /* VFS_O_* open flags      */
    i64    pos;   /* File position or cookie */
    path_s path;  /* Opened mount + dentry   */
};

/**
 * d_is_negative - Checks whether @dentry caches a failed lookup.
 * @dentry: The dentry to check.
 * Returns: 1 if @dentry caches a failed lookup, 0 otherwise.
 */
static inline int d_is_negative(const dentry_s* dentry) {
    return dentry->node == NULL;
}

/**
 * d_is_mounted - Checks whether a mount covers @dentry.
 * @dentry: The dentry to check.
 * Returns: 1 if a mount covers @dentry, 0 otherwise.
 */
static inline int d_is_mounted(const dentry_s* dentry) {
    return (dentry->flags & DENTRY_MOUNTED) != 0;
}

/**
 * d_is_dir - Checks whether @dentry is bound to a directory node.
 * @dentry: The dentry to check.
 * Returns: 1 if @dentry is bound to a directory node, 0 otherwise.
 */
static inline int d_is_dir(const dentry_s* dentry) {
    return dentry->node && VFS_ISDIR(dentry->node->mode);
}

/**
 * d_is_symlink - Checks whether @dentry is bound to a symlink node.
 * @dentry: The dentry to check.
 * Returns: 1 if @dentry is bound to a symlink node, 0 otherwise.
 */
static inline int d_is_symlink(const dentry_s* dentry) {
    return dentry->node && VFS_ISLNK(dentry->node->mode);
}
