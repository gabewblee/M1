#include <userspace/fs/lib/icache.h>
#include <userspace/fs/lib/radix.h>
#include <userspace/fs/lib/super.h>
#include <userspace/fs/tmpfs/tmpfs.h>
#include <userspace/libc/heap.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>

#define TMPFS_MAGIC     0x01021994u /* Linux tmpfs magic number       */
#define TMPFS_SEQ_FIRST 2u          /* First entry cookie after dots */

typedef struct tmpfs_inode_s tmpfs_inode_s;

typedef struct tmpfs_dirent_s {
    list_node_s    dir_list_node; /* Link in the directory  */
    tmpfs_inode_s* inode;         /* Bound child inode      */
    u32            seq;           /* Stable readdir cookie  */
    u32            len;           /* Name length            */
    char*          name;          /* NUL-terminated name    */
} tmpfs_dirent_s;

struct tmpfs_inode_s {
    fs_inode_s     vfs;      /* Embedded generic inode          */
    radix_s        pages;    /* Data page tree for regulars     */
    char*          target;   /* Symlink content                 */
    tmpfs_inode_s* parent;   /* Parent directory, self at root  */
    list_node_s    entries;  /* Directory entries               */
    u32            next_seq; /* Directory cookie allocator      */
};

static cache_s tmpfs_inode_cache;
static cache_s tmpfs_dirent_cache;
static u32     tmpfs_files;

static inline tmpfs_inode_s* tmpfs_inode(fs_inode_s* inode) {
    return get_container_of(inode, tmpfs_inode_s, vfs);
}

static void page_release(void* page) {
    pg_free(page);
}

static fs_inode_s* tmpfs_ialloc(fs_super_s* sb) {
    tmpfs_inode_s* node = cache_alloc(&tmpfs_inode_cache);
    if (!node)
        return NULL;

    radix_tree_init(&node->pages);
    node->target   = NULL;
    node->parent   = NULL;
    node->next_seq = TMPFS_SEQ_FIRST;
    list_init(&node->entries);
    tmpfs_files++;
    return &node->vfs;
}

static void tmpfs_ifree(fs_inode_s* inode) {
    tmpfs_files--;
    cache_free(&tmpfs_inode_cache, tmpfs_inode(inode));
}

static void dirent_free(tmpfs_dirent_s* entry) {
    list_del(&entry->dir_list_node);
    heap_free(entry->name, entry->len + 1u);
    cache_free(&tmpfs_dirent_cache, entry);
}

static void tmpfs_ievict(fs_inode_s* inode) {
    tmpfs_inode_s* node = tmpfs_inode(inode);
    radix_prune(&node->pages, 0, page_release);
    if (node->target) {
        heap_free(node->target, (u32)strlen(node->target) + 1u);
        node->target = NULL;
    }

    while (!list_is_empty(&node->entries))
        dirent_free(list_first_entry(&node->entries, tmpfs_dirent_s, dir_list_node));
}

static tmpfs_dirent_s* dirent_find(tmpfs_inode_s* dir, const char* name) {
    u32 len = (u32)strlen(name);
    tmpfs_dirent_s* entry;
    list_for_each_entry(entry, &dir->entries, dir_list_node) {
        if (entry->len == len && memcmp(entry->name, name, len) == 0)
            return entry;
    }

    return NULL;
}

static i32 dirent_add(tmpfs_inode_s* dir, const char* name, tmpfs_inode_s* inode) {
    u32 len = (u32)strlen(name);
    if (len >= VFS_NAME_MAX)
        return -(i32)E_NAMELONG;

    tmpfs_dirent_s* entry = cache_alloc(&tmpfs_dirent_cache);
    if (!entry)
        return -(i32)E_NOMEM;

    entry->name = heap_alloc(len + 1u);
    if (!entry->name) {
        cache_free(&tmpfs_dirent_cache, entry);
        return -(i32)E_NOMEM;
    }

    memcpy(entry->name, name, len + 1u);
    entry->inode = inode;
    entry->len   = len;
    entry->seq   = dir->next_seq++;
    list_add_to_tail(&entry->dir_list_node, &dir->entries);
    return E_OK;
}

static fs_inode_s* tmpfs_new_inode(fs_super_s* sb, u32 mode) {
    fs_inode_s* inode = fs_inode_new(sb);
    if (!inode)
        return NULL;

    inode->ino   = fs_super_ino(sb);
    inode->mode  = mode;
    inode->nlink = VFS_ISDIR(mode) ? 2u : 1u;
    fs_inode_hash(inode);
    return inode;
}

static i32 tmpfs_mknod(fs_inode_s* dir, const char* name, u32 mode, fs_inode_s** result) {
    tmpfs_inode_s* parent = tmpfs_inode(dir);
    if (dirent_find(parent, name))
        return -(i32)E_EXIST;

    fs_inode_s* inode = tmpfs_new_inode(dir->sb, mode);
    if (!inode)
        return -(i32)E_NOMEM;

    i32 ret = dirent_add(parent, name, tmpfs_inode(inode));
    if (ret != E_OK) {
        inode->nlink = 0;
        fs_iput(inode, 1);
        return ret;
    }

    if (VFS_ISDIR(mode)) {
        tmpfs_inode(inode)->parent = parent;
        dir->nlink++;
    }

    *result = inode;
    return E_OK;
}

static i32 tmpfs_lookup(fs_inode_s* dir, const char* name, fs_inode_s** result) {
    tmpfs_dirent_s* entry = dirent_find(tmpfs_inode(dir), name);
    if (!entry)
        return -(i32)E_NOENT;

    *result = fs_ihold(&entry->inode->vfs);
    return E_OK;
}

static i32 tmpfs_create(fs_inode_s* dir, const char* name, u32 mode, fs_inode_s** result) {
    return tmpfs_mknod(dir, name, mode, result);
}

static i32 tmpfs_mkdir(fs_inode_s* dir, const char* name, u32 mode, fs_inode_s** result) {
    return tmpfs_mknod(dir, name, mode, result);
}

static i32 tmpfs_rmdir(fs_inode_s* dir, const char* name) {
    tmpfs_dirent_s* entry = dirent_find(tmpfs_inode(dir), name);
    if (!entry)
        return -(i32)E_NOENT;

    fs_inode_s* child = &entry->inode->vfs;
    if (!VFS_ISDIR(child->mode))
        return -(i32)E_NOTDIR;

    if (!list_is_empty(&entry->inode->entries))
        return -(i32)E_NOTEMPTY;

    dirent_free(entry);
    child->nlink = 0;
    dir->nlink--;
    fs_iput(child, 0);
    return E_OK;
}

static i32 tmpfs_unlink(fs_inode_s* dir, const char* name) {
    tmpfs_dirent_s* entry = dirent_find(tmpfs_inode(dir), name);
    if (!entry)
        return -(i32)E_NOENT;

    fs_inode_s* child = &entry->inode->vfs;
    if (VFS_ISDIR(child->mode))
        return -(i32)E_ISDIR;

    dirent_free(entry);
    child->nlink--;
    fs_iput(child, 0);
    return E_OK;
}

static i32 tmpfs_link(fs_inode_s* inode, fs_inode_s* dir, const char* name) {
    tmpfs_inode_s* parent = tmpfs_inode(dir);
    if (VFS_ISDIR(inode->mode))
        return -(i32)E_PERM;

    if (dirent_find(parent, name))
        return -(i32)E_EXIST;

    i32 ret = dirent_add(parent, name, tmpfs_inode(inode));
    if (ret != E_OK)
        return ret;

    inode->nlink++;
    return E_OK;
}

static i32 tmpfs_symlink(fs_inode_s* dir, const char* name, const char* target, fs_inode_s** result) {
    tmpfs_inode_s* parent = tmpfs_inode(dir);
    if (dirent_find(parent, name))
        return -(i32)E_EXIST;

    u32 len = (u32)strlen(target);
    char* copy = heap_alloc(len + 1u);
    if (!copy)
        return -(i32)E_NOMEM;

    memcpy(copy, target, len + 1u);
    fs_inode_s* inode = tmpfs_new_inode(dir->sb, VFS_IFLNK | 0777u);
    if (!inode) {
        heap_free(copy, len + 1u);
        return -(i32)E_NOMEM;
    }

    i32 ret = dirent_add(parent, name, tmpfs_inode(inode));
    if (ret != E_OK) {
        heap_free(copy, len + 1u);
        inode->nlink = 0;
        fs_iput(inode, 1);
        return ret;
    }

    tmpfs_inode(inode)->target = copy;
    inode->size = len;
    *result = inode;
    return E_OK;
}

static i32 tmpfs_readlink(fs_inode_s* inode, char* buffer, u32 size) {
    const char* target = tmpfs_inode(inode)->target;
    u32 len = (u32)strlen(target);
    if (len > size)
        return -(i32)E_NAMELONG;

    memcpy(buffer, target, len);
    return (i32)len;
}

static i32 victim_unlink(fs_inode_s* to_dir, tmpfs_dirent_s* victim, int directory) {
    fs_inode_s* child = &victim->inode->vfs;
    if (VFS_ISDIR(child->mode)) {
        if (!directory)
            return -(i32)E_ISDIR;

        if (!list_is_empty(&victim->inode->entries))
            return -(i32)E_NOTEMPTY;

        dirent_free(victim);
        child->nlink = 0;
        to_dir->nlink--;
    } else {
        if (directory)
            return -(i32)E_NOTDIR;

        dirent_free(victim);
        child->nlink--;
    }

    fs_iput(child, 0);
    return E_OK;
}

static i32 tmpfs_rename(fs_inode_s* from_dir, const char* from, fs_inode_s* to_dir, const char* to) {
    tmpfs_inode_s* source = tmpfs_inode(from_dir);
    tmpfs_inode_s* target = tmpfs_inode(to_dir);

    tmpfs_dirent_s* subject = dirent_find(source, from);
    if (!subject)
        return -(i32)E_NOENT;

    tmpfs_dirent_s* victim = dirent_find(target, to);
    if (victim == subject)
        return E_OK;

    int directory = VFS_ISDIR(subject->inode->vfs.mode);
    if (victim) {
        i32 ret = victim_unlink(to_dir, victim, directory);
        if (ret != E_OK)
            return ret;
    }

    i32 ret = dirent_add(target, to, subject->inode);
    if (ret != E_OK)
        return ret;

    if (directory && source != target) {
        subject->inode->parent = target;
        from_dir->nlink--;
        to_dir->nlink++;
    }

    dirent_free(subject);
    return E_OK;
}

static i32 tmpfs_truncate(fs_inode_s* inode, i64 length) {
    tmpfs_inode_s* node = tmpfs_inode(inode);
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

static i32 tmpfs_read(fs_inode_s* inode, void* buffer, u32 count, i64 offset) {
    tmpfs_inode_s* node = tmpfs_inode(inode);
    if (offset >= inode->size)
        return 0;

    if ((i64)count > inode->size - offset)
        count = (u32)(inode->size - offset);

    u32 done = 0;
    while (done < count) {
        u32 index = (u32)(offset >> PG_SHIFT);
        u32 skip = (u32)(offset & (PG_SZ - 1));
        u32 chunk = min(count - done, PG_SZ - skip);
        u8* page = radix_lookup(&node->pages, index);
        if (page)
            memcpy((u8*)buffer + done, page + skip, chunk);
        else
            memset((u8*)buffer + done, 0, chunk);

        done += chunk;
        offset += chunk;
    }

    return (i32)done;
}

static i32 tmpfs_write(fs_inode_s* inode, const void* buffer, u32 count, i64 offset) {
    tmpfs_inode_s* node = tmpfs_inode(inode);
    u32 done = 0;
    while (done < count) {
        u32 index = (u32)(offset >> PG_SHIFT);
        u32 skip = (u32)(offset & (PG_SZ - 1));
        u32 chunk = min(count - done, PG_SZ - skip);
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

        memcpy(page + skip, (const u8*)buffer + done, chunk);
        done += chunk;
        offset += chunk;
    }

    if (offset > inode->size)
        inode->size = offset;

    return done ? (i32)done : -(i32)E_NOSPC;
}

static i32 tmpfs_readdir(fs_inode_s* dir, u32 cookie, fs_readdir_reply_s* reply) {
    tmpfs_inode_s* node = tmpfs_inode(dir);
    if (cookie == 0) {
        reply->cookie = 1;
        reply->ino    = dir->ino;
        reply->type   = VFS_DT_DIR;
        strlcpy(reply->name, ".", sizeof(reply->name));
        return 1;
    }

    if (cookie == 1) {
        reply->cookie = TMPFS_SEQ_FIRST;
        reply->ino    = node->parent->vfs.ino;
        reply->type   = VFS_DT_DIR;
        strlcpy(reply->name, "..", sizeof(reply->name));
        return 1;
    }

    tmpfs_dirent_s* entry;
    list_for_each_entry(entry, &node->entries, dir_list_node) {
        if (entry->seq < cookie)
            continue;

        reply->cookie = entry->seq + 1u;
        reply->ino    = entry->inode->vfs.ino;
        reply->type   = VFS_DT(entry->inode->vfs.mode & VFS_IFMT);
        strlcpy(reply->name, entry->name, sizeof(reply->name));
        return 1;
    }

    return 0;
}

static i32 tmpfs_mount(fs_super_s* sb, const char* source, u32 flags) {
    fs_inode_s* root = tmpfs_new_inode(sb, VFS_IFDIR | 0755u);
    if (!root)
        return -(i32)E_NOMEM;

    tmpfs_inode(root)->parent = tmpfs_inode(root);
    sb->flags     = FS_SUPER_KEEP_CACHE;
    sb->blocksize = PG_SZ;
    sb->magic     = TMPFS_MAGIC;
    sb->root      = root;
    return E_OK;
}

static void tmpfs_statfs(fs_super_s* sb, vfs_statfs_s* statfs) {
    statfs->blocks    = HEAP_ARENA_PG;
    statfs->bfree     = heap_pages_free();
    statfs->files     = tmpfs_files;
    statfs->ffree     = tmpfs_inode_cache.total - tmpfs_inode_cache.used;
    statfs->blocksize = sb->blocksize;
}

static i32 tmpfs_init(void) {
    radix_init();
    cache_init(&tmpfs_inode_cache, "tmpfs-inode", sizeof(tmpfs_inode_s));
    cache_init(&tmpfs_dirent_cache, "tmpfs-dirent", sizeof(tmpfs_dirent_s));
    return E_OK;
}

const fs_driver_s tmpfs_driver = {
    .name     = "tmpfs",
    .init     = tmpfs_init,
    .mount    = tmpfs_mount,
    .statfs   = tmpfs_statfs,
    .lookup   = tmpfs_lookup,
    .create   = tmpfs_create,
    .mkdir    = tmpfs_mkdir,
    .rmdir    = tmpfs_rmdir,
    .unlink   = tmpfs_unlink,
    .link     = tmpfs_link,
    .symlink  = tmpfs_symlink,
    .readlink = tmpfs_readlink,
    .rename   = tmpfs_rename,
    .truncate = tmpfs_truncate,
    .read     = tmpfs_read,
    .write    = tmpfs_write,
    .readdir  = tmpfs_readdir,
    .ialloc   = tmpfs_ialloc,
    .ifree    = tmpfs_ifree,
    .ievict   = tmpfs_ievict,
};
