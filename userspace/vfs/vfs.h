#pragma once

#include <uapi/uapi.h>
#include <userspace/libc/list.h>

#define VFS_SYMLINK_MAX 8u   /* Nested symlink resolution limit */
#define VFS_CLIENT_MAX  9u   /* Badge-indexed client table size */
#define VFS_FD_MAX      32u  /* Descriptors per client          */
#define DENTRY_INLINE   32u  /* Inline dentry name bytes        */
#define DENTRY_MOUNTED  0x1u /* Dentry is a mountpoint          */
#define DENTRY_ONLRU    0x2u /* Dentry is on the LRU list       */

typedef struct qstr_s        qstr_s;
typedef struct dentry_s      dentry_s;
typedef struct inode_s       inode_s;
typedef struct file_s        file_s;
typedef struct mount_s       mount_s;
typedef struct path_s        path_s;
typedef struct super_block_s super_block_s;
typedef struct fstype_s      fstype_s;
typedef struct inode_ops_s   inode_ops_s;
typedef struct file_ops_s    file_ops_s;
typedef struct super_ops_s   super_ops_s;

struct qstr_s {
    const char* name; /* Component bytes  */
    u32         len;  /* Component length */
    u32         hash; /* Full name hash   */
};

struct path_s {
    mount_s*  mount;  /* Mount the dentry was reached through */
    dentry_s* dentry; /* Dentry within the mount              */
};

struct dentry_s {
    u32            count;                /* Reference count                 */
    u32            flags;                /* DENTRY_* flags                  */
    dentry_s*      parent;               /* Parent dentry, self at root     */
    inode_s*       inode;                /* Bound inode, NULL when negative */
    super_block_s* sb;                   /* Owning superblock               */
    qstr_s         name;                 /* Component name                  */
    char           iname[DENTRY_INLINE]; /* Inline short-name storage       */
    hlist_node_s   hash_list_node;       /* Dentry hash chain link          */
    list_node_s    child_list_node;      /* Sibling link in parent          */
    list_node_s    children;             /* Child dentries                  */
    list_node_s    lru_list_node;        /* LRU link when unreferenced      */
};

struct inode_s {
    u32                ino;            /* Inode number            */
    u32                mode;           /* Type and mode bits      */
    u32                nlink;          /* Hard link count         */
    u32                count;          /* Reference count         */
    i64                size;           /* Size in bytes           */
    u32                blocks;         /* Allocated page count    */
    super_block_s*     sb;             /* Owning superblock       */
    const inode_ops_s* ops;            /* Inode operations        */
    const file_ops_s*  fops;           /* Default file operations */
    void*              private;        /* Filesystem-private data */
    hlist_node_s       hash_list_node; /* Inode hash chain link   */
    list_node_s        sb_list_node;   /* Link in superblock list */
};

struct super_block_s {
    u32             id;                /* Device identifier       */
    u32             magic;             /* Filesystem magic number */
    u32             blocksize;         /* Block size in bytes     */
    u32             next_inode_number; /* Inode number allocator  */
    const fstype_s* fstype;            /* Filesystem type         */
    dentry_s*       root;              /* Root dentry             */
    const super_ops_s* ops;            /* Superblock operations   */
    list_node_s     inodes;            /* In-core inodes          */
    void*           private;           /* Filesystem-private data */
};

struct fstype_s {
    const char* name;                                                      /* Filesystem name     */
    u32         flags;                                                     /* Filesystem flags    */
    dentry_s*   (*mount)(fstype_s* fstype, u32 flags, const char* source); /* Builds a superblock */
    void        (*kill)(super_block_s* sb);                                /* Tears the tree down */
    fstype_s*   next;                                                      /* Registry chain link */
};

struct mount_s {
    u32          count;           /* Reference count              */
    mount_s*     parent;          /* Parent mount, NULL at root   */
    dentry_s*    mountpoint;      /* Covered dentry in the parent */
    dentry_s*    root;            /* Root dentry of this mount    */
    super_block_s* sb;            /* Mounted superblock           */
    hlist_node_s hash_list_node;  /* Mount hash chain link        */
    list_node_s  child_list_node; /* Sibling link in parent       */
    list_node_s  children;        /* Mounts on top of this mount  */
};

struct file_s {
    u32               count;   /* Reference count       */
    u32               flags;   /* VFS_O_* open flags    */
    i64               pos;     /* File position         */
    path_s            path;    /* Opened mount + dentry */
    const file_ops_s* ops;     /* File operations       */
    void*             private; /* Per-open private data */
};

struct inode_ops_s {
    dentry_s*   (*lookup)(inode_s* dir, dentry_s* dentry);                                 /* Resolves one component */
    i32         (*create)(inode_s* dir, dentry_s* dentry, u32 mode);                       /* Creates a regular file */
    i32         (*mkdir)(inode_s* dir, dentry_s* dentry, u32 mode);                        /* Creates a directory    */
    i32         (*rmdir)(inode_s* dir, dentry_s* dentry);                                  /* Removes a directory    */
    i32         (*unlink)(inode_s* dir, dentry_s* dentry);                                 /* Removes a link         */
    i32         (*link)(dentry_s* source, inode_s* dir, dentry_s* dentry);                 /* Adds a hard link       */
    i32         (*symlink)(inode_s* dir, dentry_s* dentry, const char* content);           /* Creates a symlink      */
    i32         (*rename)(inode_s* source, dentry_s* from, inode_s* target, dentry_s* to); /* Moves an entry         */
    i32         (*truncate)(inode_s* inode, i64 length);                                   /* Resizes a file         */
    const char* (*getlink)(inode_s* inode);                                                /* Reads symlink content  */
};

struct file_ops_s {
    i32 (*open)(inode_s* inode, file_s* file);                                /* Per-open setup       */
    i32 (*release)(inode_s* inode, file_s* file);                             /* Last-reference close */
    i32 (*read)(file_s* file, void* buffer, u32 count, i64* position);        /* Reads at *position   */
    i32 (*write)(file_s* file, const void* buffer, u32 count, i64* position); /* Writes at *position  */
    i32 (*llseek)(file_s* file, i64 offset, u32 whence, i64* result);         /* Repositions the file */
    i32 (*readdir)(file_s* file, vfs_dirent_s* entry);                        /* Emits one entry      */
};

struct super_ops_s {
    inode_s* (*alloc)(super_block_s* sb); /* Allocates an in-core inode */
    void     (*destroy)(inode_s* inode);  /* Frees an in-core inode     */
    void     (*evict)(inode_s* inode);    /* Drops inode data           */
};

/**
 * d_is_negative - Checks whether @dentry caches a failed lookup.
 * @dentry: The dentry to check.
 * Returns: 1 if @dentry caches a failed lookup, 0 otherwise.
 */
static inline int d_is_negative(const dentry_s* dentry) {
    return dentry->inode == NULL;
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
 * d_is_dir - Checks whether @dentry is bound to a directory inode.
 * @dentry: The dentry to check.
 * Returns: 1 if @dentry is bound to a directory inode, 0 otherwise.
 */
static inline int d_is_dir(const dentry_s* dentry) {
    return dentry->inode && VFS_ISDIR(dentry->inode->mode);
}
