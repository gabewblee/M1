#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>
#include <userspace/vfs/dcache.h>
#include <userspace/vfs/hash.h>
#include <userspace/vfs/heap.h>
#include <userspace/vfs/inode.h>
#include <userspace/vfs/libfs.h>
#include <userspace/vfs/radix.h>
#include <userspace/vfs/ramfs.h>
#include <userspace/vfs/super.h>

#define RAMFS_MAGIC 0x858458F6u

typedef struct ramfs_inode_s {
    inode_s vfs;    /* Embedded VFS inode          */
    radix_s pages;  /* Data page tree for regulars */
    char*   target; /* Symlink content             */
} ramfs_inode_s;

static cache_s ramfs_inode_cache;

static const inode_ops_s ramfs_dir_ops;
static const inode_ops_s ramfs_reg_ops;
static const inode_ops_s ramfs_link_ops;
static const file_ops_s  ramfs_file_ops;

static inline ramfs_inode_s* ramfs_inode(inode_s* inode) {
    return get_container_of(inode, ramfs_inode_s, vfs);
}

static void page_release(void* page) {
    pg_free(page);
}

static inode_s* ramfs_alloc(super_block_s* sb) {
    ramfs_inode_s* node = cache_alloc(&ramfs_inode_cache);
    if (!node)
        return NULL;

    radix_tree_init(&node->pages);
    node->target = NULL;
    return &node->vfs;
}

static void ramfs_destroy(inode_s* inode) {
    cache_free(&ramfs_inode_cache, ramfs_inode(inode));
}

static void ramfs_evict(inode_s* inode) {
    ramfs_inode_s* node = ramfs_inode(inode);
    radix_prune(&node->pages, 0, page_release);
    if (node->target) {
        heap_free(node->target, (u32)strlen(node->target) + 1u);
        node->target = NULL;
    }
}

static const super_ops_s ramfs_super_ops = {
    .alloc   = ramfs_alloc,
    .destroy = ramfs_destroy,
    .evict   = ramfs_evict,
};

static inode_s* ramfs_iget(super_block_s* sb, u32 mode) {
    inode_s* inode = inode_alloc(sb);
    if (!inode)
        return NULL;

    inode->ino   = super_ino(sb);
    inode->mode  = mode;
    inode->nlink = 1;
    switch (mode & VFS_IFMT) {
        case VFS_IFDIR:
            inode->ops   = &ramfs_dir_ops;
            inode->fops  = &simple_dir_ops;
            inode->nlink = 2;
            break;

        case VFS_IFREG:
            inode->ops  = &ramfs_reg_ops;
            inode->fops = &ramfs_file_ops;
            break;

        case VFS_IFLNK:
            inode->ops = &ramfs_link_ops;
            break;
    }

    inode_insert(inode);
    return inode;
}

static i32 ramfs_mknod(inode_s* dir, dentry_s* dentry, u32 mode) {
    inode_s* inode = ramfs_iget(dir->sb, mode);
    if (!inode)
        return -(i32)E_NOMEM;

    d_instantiate(dentry, inode);
    dget(dentry);
    if (VFS_ISDIR(mode))
        dir->nlink++;

    return E_OK;
}

static i32 ramfs_create(inode_s* dir, dentry_s* dentry, u32 mode) {
    return ramfs_mknod(dir, dentry, mode);
}

static i32 ramfs_mkdir(inode_s* dir, dentry_s* dentry, u32 mode) {
    return ramfs_mknod(dir, dentry, mode);
}

static i32 ramfs_link(dentry_s* source, inode_s* dir, dentry_s* dentry) {
    inode_s* inode = source->inode;
    inode->nlink++;
    d_instantiate(dentry, ihold(inode));
    dget(dentry);
    return E_OK;
}

static i32 ramfs_symlink(inode_s* dir, dentry_s* dentry, const char* content) {
    u32 length = (u32)strlen(content);
    char* copy = heap_alloc(length + 1u);
    if (!copy)
        return -(i32)E_NOMEM;

    memcpy(copy, content, length + 1u);
    inode_s* inode = ramfs_iget(dir->sb, VFS_IFLNK | 0777u);
    if (!inode) {
        heap_free(copy, length + 1u);
        return -(i32)E_NOMEM;
    }

    ramfs_inode(inode)->target = copy;
    inode->size = length;
    d_instantiate(dentry, inode);
    dget(dentry);
    return E_OK;
}

static const char* ramfs_getlink(inode_s* inode) {
    return ramfs_inode(inode)->target;
}

static i32 ramfs_truncate(inode_s* inode, i64 length) {
    ramfs_inode_s* node = ramfs_inode(inode);
    if (length < inode->size) {
        u32 first = (u32)((length + PG_SZ - 1) >> PG_SHIFT);
        inode->blocks -= radix_prune(&node->pages, first, page_release);

        u32 offset = (u32)(length & (PG_SZ - 1));
        if (offset) {
            u8* page = radix_lookup(&node->pages, (u32)(length >> PG_SHIFT));
            if (page)
                memset(page + offset, 0, PG_SZ - offset);
        }
    }

    inode->size = length;
    return E_OK;
}

static i32 ramfs_read(file_s* file, void* buffer, u32 count, i64* position) {
    inode_s* inode = file->path.dentry->inode;
    ramfs_inode_s* node = ramfs_inode(inode);
    if (*position >= inode->size)
        return 0;

    if ((i64)count > inode->size - *position)
        count = (u32)(inode->size - *position);

    u32 done = 0;
    while (done < count) {
        u32 index = (u32)(*position >> PG_SHIFT);
        u32 offset = (u32)(*position & (PG_SZ - 1));
        u32 chunk = min(count - done, PG_SZ - offset);
        u8* page = radix_lookup(&node->pages, index);
        if (page)
            memcpy((u8*)buffer + done, page + offset, chunk);
        else
            memset((u8*)buffer + done, 0, chunk);

        done += chunk;
        *position += chunk;
    }

    return (i32)done;
}

static i32 ramfs_write(file_s* file, const void* buffer, u32 count, i64* position) {
    inode_s* inode = file->path.dentry->inode;
    ramfs_inode_s* node = ramfs_inode(inode);
    u32 done = 0;
    while (done < count) {
        u32 index = (u32)(*position >> PG_SHIFT);
        u32 offset = (u32)(*position & (PG_SZ - 1));
        u32 chunk = min(count - done, PG_SZ - offset);
        u8* page = radix_lookup(&node->pages, index);
        if (!page) {
            page = pg_alloc();
            if (!page)
                break;

            memset(page, 0, PG_SZ);
            if (radix_insert(&node->pages, index, page) != E_OK) {
                pg_free(page);
                break;
            }

            inode->blocks++;
        }

        memcpy(page + offset, (const u8*)buffer + done, chunk);
        done += chunk;
        *position += chunk;
    }

    if (*position > inode->size)
        inode->size = *position;

    return done ? (i32)done : -(i32)E_NOSPC;
}

static const inode_ops_s ramfs_dir_ops = {
    .lookup  = simple_lookup,
    .create  = ramfs_create,
    .mkdir   = ramfs_mkdir,
    .rmdir   = simple_rmdir,
    .unlink  = simple_unlink,
    .link    = ramfs_link,
    .symlink = ramfs_symlink,
    .rename  = simple_rename,
};

static const inode_ops_s ramfs_reg_ops = {
    .truncate = ramfs_truncate,
};

static const inode_ops_s ramfs_link_ops = {
    .getlink = ramfs_getlink,
};

static const file_ops_s ramfs_file_ops = {
    .read   = ramfs_read,
    .write  = ramfs_write,
    .llseek = generic_seek,
};

static dentry_s* ramfs_mount(fstype_s* fstype, u32 flags, const char* source) {
    super_block_s* sb = super_alloc(fstype);
    if (!sb)
        return NULL;

    sb->magic = RAMFS_MAGIC;
    sb->ops   = &ramfs_super_ops;

    inode_s* inode = ramfs_iget(sb, VFS_IFDIR | 0755u);
    if (!inode) {
        super_kill(sb);
        return NULL;
    }

    qstr_s name = { "/", 1, hash_full_name("/", 1) };
    dentry_s* dentry = d_alloc(NULL, &name);
    if (!dentry) {
        inode->nlink = 0;
        iput(inode);
        super_kill(sb);
        return NULL;
    }

    dentry->sb = sb;
    d_instantiate(dentry, inode);
    sb->root = dentry;
    return dentry;
}

static void ramfs_kill(super_block_s* sb) {
    if (!sb->root)
        return;

    d_genocide(sb->root);
    dput(sb->root);
    sb->root = NULL;
    dcache_shrink(SHRINK_ALL);
}

static fstype_s ramfs = {
    .name  = "ramfs",
    .mount = ramfs_mount,
    .kill  = ramfs_kill,
};

void ramfs_init(void) {
    cache_init(&ramfs_inode_cache, "ramfs-inode", sizeof(ramfs_inode_s));
    fstype_register(&ramfs);
}
