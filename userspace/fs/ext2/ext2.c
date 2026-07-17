#include <userspace/fs/ext2/blk.h>
#include <userspace/fs/ext2/ext2.h>
#include <userspace/fs/lib/icache.h>
#include <userspace/libc/heap.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>

static cache_s ext2_inode_cache;
static int     ext2_mounted;

static u8 ft_of(u32 mode) {
    if (VFS_ISDIR(mode))
        return EXT2_FT_DIR;

    if (VFS_ISLNK(mode))
        return EXT2_FT_SYMLINK;

    return EXT2_FT_REG;
}

static int is_fast_symlink(const fs_inode_s* inode) {
    return VFS_ISLNK(inode->mode) && inode->blocks == 0;
}

static fs_inode_s* ext2_ialloc_cb(fs_super_s* sb) {
    ext2_inode_s* node = cache_alloc(&ext2_inode_cache);
    if (!node)
        return NULL;

    memset(node->data, 0, sizeof(node->data));
    return &node->vfs;
}

static void ext2_ifree_cb(fs_inode_s* inode) {
    cache_free(&ext2_inode_cache, ext2_inode_of(inode));
}

static void ext2_ievict_cb(fs_inode_s* inode) {
    if (inode->nlink)
        return;

    if (!is_fast_symlink(inode))
        ext2_ifree_blocks(inode);

    ext2_iwrite(inode);
    ext2_inode_free(ext2_fs_of(inode->sb), inode->ino, VFS_ISDIR(inode->mode));
}

static i32 inode_make(fs_super_s* sb, u32 mode, fs_inode_s** result) {
    ext2_fs_s* fs = ext2_fs_of(sb);
    u32 ino;
    i32 ret = ext2_inode_alloc(fs, VFS_ISDIR(mode), &ino);
    if (ret != E_OK)
        return ret;

    fs_inode_s* inode = fs_inode_new(sb);
    if (!inode) {
        ext2_inode_free(fs, ino, VFS_ISDIR(mode));
        return -(i32)E_NOMEM;
    }

    inode->ino   = ino;
    inode->mode  = mode;
    inode->nlink = VFS_ISDIR(mode) ? 2u : 1u;
    fs_inode_hash(inode);

    ret = ext2_iwrite(inode);
    if (ret != E_OK) {
        inode->nlink = 0;
        fs_iput(inode, 1);
        return ret;
    }

    *result = inode;
    return E_OK;
}

static void inode_undo(fs_inode_s* inode) {
    inode->nlink = 0;
    fs_iput(inode, 1);
}

static i32 ext2_lookup_cb(fs_inode_s* dir, const char* name, fs_inode_s** result) {
    u32 ino;
    i32 ret = ext2_dir_find(dir, name, &ino);
    if (ret != E_OK)
        return ret;

    return ext2_iget(dir->sb, ino, result);
}

static i32 ext2_create_cb(fs_inode_s* dir, const char* name, u32 mode, fs_inode_s** result) {
    u32 existing;
    if (ext2_dir_find(dir, name, &existing) == E_OK)
        return -(i32)E_EXIST;

    fs_inode_s* inode;
    i32 ret = inode_make(dir->sb, mode, &inode);
    if (ret != E_OK)
        return ret;

    ret = ext2_dir_add(dir, name, inode->ino, ft_of(mode));
    if (ret != E_OK) {
        inode_undo(inode);
        return ret;
    }

    *result = inode;
    return E_OK;
}

static i32 ext2_mkdir_cb(fs_inode_s* dir, const char* name, u32 mode, fs_inode_s** result) {
    u32 existing;
    if (ext2_dir_find(dir, name, &existing) == E_OK)
        return -(i32)E_EXIST;

    fs_inode_s* inode;
    i32 ret = inode_make(dir->sb, mode, &inode);
    if (ret != E_OK)
        return ret;

    ret = ext2_dir_add(inode, ".", inode->ino, EXT2_FT_DIR);
    if (ret == E_OK)
        ret = ext2_dir_add(inode, "..", dir->ino, EXT2_FT_DIR);

    if (ret == E_OK)
        ret = ext2_dir_add(dir, name, inode->ino, EXT2_FT_DIR);

    if (ret != E_OK) {
        inode_undo(inode);
        return ret;
    }

    dir->nlink++;
    ext2_iwrite(dir);
    *result = inode;
    return E_OK;
}

static i32 ext2_rmdir_cb(fs_inode_s* dir, const char* name) {
    u32 ino;
    i32 ret = ext2_dir_find(dir, name, &ino);
    if (ret != E_OK)
        return ret;

    fs_inode_s* child;
    ret = ext2_iget(dir->sb, ino, &child);
    if (ret != E_OK)
        return ret;

    if (!VFS_ISDIR(child->mode)) {
        fs_iput(child, 1);
        return -(i32)E_NOTDIR;
    }

    ret = ext2_dir_is_empty(child);
    if (ret <= 0) {
        fs_iput(child, 1);
        return (ret < 0) ? ret : -(i32)E_NOTEMPTY;
    }

    ret = ext2_dir_remove(dir, name);
    if (ret != E_OK) {
        fs_iput(child, 1);
        return ret;
    }

    child->nlink = 0;
    dir->nlink--;
    ext2_iwrite(child);
    ext2_iwrite(dir);
    fs_iput(child, 1);
    return E_OK;
}

static i32 ext2_unlink_cb(fs_inode_s* dir, const char* name) {
    u32 ino;
    i32 ret = ext2_dir_find(dir, name, &ino);
    if (ret != E_OK)
        return ret;

    fs_inode_s* child;
    ret = ext2_iget(dir->sb, ino, &child);
    if (ret != E_OK)
        return ret;

    if (VFS_ISDIR(child->mode)) {
        fs_iput(child, 1);
        return -(i32)E_ISDIR;
    }

    ret = ext2_dir_remove(dir, name);
    if (ret != E_OK) {
        fs_iput(child, 1);
        return ret;
    }

    child->nlink--;
    ext2_iwrite(child);
    fs_iput(child, 1);
    return E_OK;
}

static i32 ext2_link_cb(fs_inode_s* inode, fs_inode_s* dir, const char* name) {
    if (VFS_ISDIR(inode->mode))
        return -(i32)E_PERM;

    u32 existing;
    if (ext2_dir_find(dir, name, &existing) == E_OK)
        return -(i32)E_EXIST;

    i32 ret = ext2_dir_add(dir, name, inode->ino, ft_of(inode->mode));
    if (ret != E_OK)
        return ret;

    inode->nlink++;
    return ext2_iwrite(inode);
}

static i32 ext2_symlink_cb(fs_inode_s* dir, const char* name, const char* target,
                           fs_inode_s** result) {
    u32 existing;
    if (ext2_dir_find(dir, name, &existing) == E_OK)
        return -(i32)E_EXIST;

    u32 len = (u32)strlen(target);
    if (len >= EXT2_FAST_LINK_MAX)
        return -(i32)E_NAMELONG;

    fs_inode_s* inode;
    i32 ret = inode_make(dir->sb, VFS_IFLNK | 0777u, &inode);
    if (ret != E_OK)
        return ret;

    memcpy(ext2_inode_of(inode)->data, target, len);
    inode->size = len;
    ret = ext2_iwrite(inode);
    if (ret == E_OK)
        ret = ext2_dir_add(dir, name, inode->ino, EXT2_FT_SYMLINK);

    if (ret != E_OK) {
        inode_undo(inode);
        return ret;
    }

    *result = inode;
    return E_OK;
}

static i32 ext2_readlink_cb(fs_inode_s* inode, char* buffer, u32 size) {
    u32 len = min((u32)inode->size, size);
    if (is_fast_symlink(inode)) {
        memcpy(buffer, ext2_inode_of(inode)->data, len);
        return (i32)len;
    }

    return ext2_file_read(inode, buffer, len, 0);
}

static i32 victim_unlink(fs_inode_s* to_dir, fs_inode_s* victim, const char* to, int directory) {
    if (VFS_ISDIR(victim->mode)) {
        if (!directory)
            return -(i32)E_ISDIR;

        i32 ret = ext2_dir_is_empty(victim);
        if (ret <= 0)
            return (ret < 0) ? ret : -(i32)E_NOTEMPTY;
    } else if (directory) {
        return -(i32)E_NOTDIR;
    }

    i32 ret = ext2_dir_remove(to_dir, to);
    if (ret != E_OK)
        return ret;

    if (VFS_ISDIR(victim->mode)) {
        victim->nlink = 0;
        to_dir->nlink--;
        ext2_iwrite(to_dir);
    } else {
        victim->nlink--;
    }

    ext2_iwrite(victim);
    return E_OK;
}

static i32 ext2_rename_cb(fs_inode_s* from_dir, const char* from, fs_inode_s* to_dir,
                          const char* to) {
    u32 ino;
    i32 ret = ext2_dir_find(from_dir, from, &ino);
    if (ret != E_OK)
        return ret;

    fs_inode_s* subject;
    ret = ext2_iget(from_dir->sb, ino, &subject);
    if (ret != E_OK)
        return ret;

    int directory = VFS_ISDIR(subject->mode);
    u32 victim_ino;
    if (ext2_dir_find(to_dir, to, &victim_ino) == E_OK) {
        if (victim_ino == ino) {
            fs_iput(subject, 1);
            return E_OK;
        }

        fs_inode_s* victim;
        ret = ext2_iget(to_dir->sb, victim_ino, &victim);
        if (ret != E_OK) {
            fs_iput(subject, 1);
            return ret;
        }

        ret = victim_unlink(to_dir, victim, to, directory);
        fs_iput(victim, 1);
        if (ret != E_OK) {
            fs_iput(subject, 1);
            return ret;
        }
    }

    ret = ext2_dir_add(to_dir, to, ino, ft_of(subject->mode));
    if (ret == E_OK)
        ret = ext2_dir_remove(from_dir, from);

    if (ret == E_OK && directory && from_dir != to_dir) {
        ext2_dir_set_parent(subject, to_dir->ino);
        from_dir->nlink--;
        to_dir->nlink++;
        ext2_iwrite(from_dir);
        ext2_iwrite(to_dir);
    }

    fs_iput(subject, 1);
    return ret;
}

static i32 ext2_truncate_cb(fs_inode_s* inode, i64 length) {
    return ext2_itruncate(inode, length);
}

static i32 ext2_read_cb(fs_inode_s* inode, void* buffer, u32 count, i64 offset) {
    return ext2_file_read(inode, buffer, count, offset);
}

static i32 ext2_write_cb(fs_inode_s* inode, const void* buffer, u32 count, i64 offset) {
    return ext2_file_write(inode, buffer, count, offset);
}

static i32 ext2_readdir_cb(fs_inode_s* dir, u32 cookie, fs_readdir_reply_s* reply) {
    return ext2_readdir(dir, cookie, reply);
}

static u32 parse_drive(const char* source) {
    u32 len = (u32)strlen(source);
    if (len && source[len - 1u] >= '0' && source[len - 1u] <= '9')
        return (u32)(source[len - 1u] - '0');

    return 0;
}

static i32 ext2_mount_cb(fs_super_s* sb, const char* source, u32 flags) {
    if (ext2_mounted)
        return -(i32)E_BUSY;

    i32 ret = blk_init(parse_drive(source));
    if (ret != E_OK)
        return ret;

    static u8 raw[EXT2_SB_SZ];
    ret = blk_read(EXT2_SB_OFF / 512u, EXT2_SB_SZ / 512u, raw);
    if (ret != E_OK)
        return ret;

    const ext2_dsuper_s* dsb = (const ext2_dsuper_s*)raw;
    if (dsb->s_magic != EXT2_MAGIC)
        return -(i32)E_INVAL;

    u32 block_log = EXT2_MIN_BLOCK_LOG + dsb->s_log_block_size;
    if (block_log > EXT2_MAX_BLOCK_LOG)
        return -(i32)E_INVAL;

    ext2_fs_s* fs = heap_alloc(sizeof(*fs));
    if (!fs)
        return -(i32)E_NOMEM;

    memcpy(&fs->dsb, dsb, sizeof(fs->dsb));
    fs->sb         = sb;
    fs->block_size = 1u << block_log;
    fs->block_log  = block_log;
    fs->ptrs       = fs->block_size / (u32)sizeof(u32);
    fs->gd_block   = fs->dsb.s_first_data_block + 1u;
    fs->inode_size = (fs->dsb.s_rev_level == EXT2_GOOD_OLD_REV)
        ? EXT2_GOOD_OLD_ISIZE
        : fs->dsb.s_inode_size;

    u32 data   = fs->dsb.s_blocks_count - fs->dsb.s_first_data_block;
    fs->groups = (data + fs->dsb.s_blocks_per_group - 1u) / fs->dsb.s_blocks_per_group;

    ret = bcache_init(fs->block_size);
    if (ret != E_OK) {
        heap_free(fs, sizeof(*fs));
        return ret;
    }

    sb->blocksize = fs->block_size;
    sb->magic     = EXT2_MAGIC;
    sb->private   = fs;

    fs_inode_s* root;
    ret = ext2_iget(sb, EXT2_ROOT_INO, &root);
    if (ret != E_OK) {
        heap_free(fs, sizeof(*fs));
        return ret;
    }

    sb->root     = root;
    ext2_mounted = 1;
    return E_OK;
}

static void ext2_umount_cb(fs_super_s* sb) {
    bcache_drop();
    heap_free(sb->private, sizeof(ext2_fs_s));
    ext2_mounted = 0;
}

static i32 ext2_sync_cb(fs_super_s* sb) {
    return bcache_sync();
}

static void ext2_statfs_cb(fs_super_s* sb, vfs_statfs_s* statfs) {
    const ext2_fs_s* fs = ext2_fs_of(sb);
    statfs->blocks    = fs->dsb.s_blocks_count;
    statfs->bfree     = fs->dsb.s_free_blocks_count;
    statfs->files     = fs->dsb.s_inodes_count;
    statfs->ffree     = fs->dsb.s_free_inodes_count;
    statfs->blocksize = fs->block_size;
}

static i32 ext2_init_cb(void) {
    cache_init(&ext2_inode_cache, "ext2-inode", sizeof(ext2_inode_s));
    return E_OK;
}

const fs_driver_s ext2_driver = {
    .name     = "ext2",
    .init     = ext2_init_cb,
    .mount    = ext2_mount_cb,
    .umount   = ext2_umount_cb,
    .sync     = ext2_sync_cb,
    .statfs   = ext2_statfs_cb,
    .lookup   = ext2_lookup_cb,
    .create   = ext2_create_cb,
    .mkdir    = ext2_mkdir_cb,
    .rmdir    = ext2_rmdir_cb,
    .unlink   = ext2_unlink_cb,
    .link     = ext2_link_cb,
    .symlink  = ext2_symlink_cb,
    .readlink = ext2_readlink_cb,
    .rename   = ext2_rename_cb,
    .truncate = ext2_truncate_cb,
    .read     = ext2_read_cb,
    .write    = ext2_write_cb,
    .readdir  = ext2_readdir_cb,
    .ialloc   = ext2_ialloc_cb,
    .ifree    = ext2_ifree_cb,
    .ievict   = ext2_ievict_cb,
};
