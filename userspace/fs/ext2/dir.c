#include <userspace/fs/ext2/ext2.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>

#define DIR_SCAN_DIRTY 1 /* Stop scanning, block was modified */
#define DIR_SCAN_FOUND 2 /* Stop scanning, block is untouched */

typedef i32 (*dir_visit_f)(ext2_dirent_s* entry, ext2_dirent_s* previous, u32 offset,
                           void* context);

static i32 dir_scan(fs_inode_s* dir, dir_visit_f visit, void* context) {
    ext2_fs_s* fs = ext2_fs_of(dir->sb);
    u32 blocks = (u32)((dir->size + fs->block_size - 1) >> fs->block_log);

    for (u32 index = 0; index < blocks; index++) {
        u32 block;
        i32 ret = ext2_bmap(dir, index, 0, &block);
        if (ret != E_OK)
            return ret;

        if (!block)
            continue;

        buf_s* buffer = bread(block);
        if (!buffer)
            return -(i32)E_IO;

        ext2_dirent_s* previous = NULL;
        u32 offset = 0;
        while (offset + EXT2_DIRENT_BASE <= fs->block_size) {
            ext2_dirent_s* entry = (ext2_dirent_s*)(buffer->data + offset);
            if (entry->rec_len < EXT2_DIRENT_BASE || offset + entry->rec_len > fs->block_size) {
                brelse(buffer);
                return -(i32)E_IO;
            }

            ret = visit(entry, previous, index * fs->block_size + offset, context);
            if (ret) {
                if (ret == DIR_SCAN_DIRTY)
                    bdirty(buffer);

                brelse(buffer);
                return ret;
            }

            previous = entry;
            offset += entry->rec_len;
        }

        brelse(buffer);
    }

    return 0;
}

typedef struct dir_name_ctx_s {
    const char* name;      /* Component operated on */
    u32         len;       /* Component length      */
    u32         ino;       /* Entry inode number    */
    u8          file_type; /* One of EXT2_FT_*      */
} dir_name_ctx_s;

static int name_matches(const ext2_dirent_s* entry, const dir_name_ctx_s* ctx) {
    return entry->inode && entry->name_len == ctx->len &&
           memcmp(entry->name, ctx->name, ctx->len) == 0;
}

static i32 find_visit(ext2_dirent_s* entry, ext2_dirent_s* previous, u32 offset, void* context) {
    dir_name_ctx_s* ctx = context;
    if (!name_matches(entry, ctx))
        return 0;

    ctx->ino = entry->inode;
    return DIR_SCAN_FOUND;
}

i32 ext2_dir_find(fs_inode_s* dir, const char* name, u32* result) {
    dir_name_ctx_s ctx = { name, (u32)strlen(name), 0, 0 };
    i32 ret = dir_scan(dir, find_visit, &ctx);
    if (ret == DIR_SCAN_FOUND) {
        *result = ctx.ino;
        return E_OK;
    }

    return (ret < 0) ? ret : -(i32)E_NOENT;
}

static void dirent_fill(ext2_dirent_s* entry, const dir_name_ctx_s* ctx, u16 rec_len) {
    entry->inode     = ctx->ino;
    entry->rec_len   = rec_len;
    entry->name_len  = (u8)ctx->len;
    entry->file_type = ctx->file_type;
    memcpy(entry->name, ctx->name, ctx->len);
}

static i32 add_visit(ext2_dirent_s* entry, ext2_dirent_s* previous, u32 offset, void* context) {
    dir_name_ctx_s* ctx = context;
    u32 need = EXT2_DIRENT_SZ(ctx->len);

    if (!entry->inode && entry->rec_len >= need) {
        dirent_fill(entry, ctx, entry->rec_len);
        return DIR_SCAN_DIRTY;
    }

    u32 used = EXT2_DIRENT_SZ(entry->name_len);
    if (entry->inode && entry->rec_len >= used + need) {
        u16 rest = (u16)(entry->rec_len - used);
        entry->rec_len = (u16)used;
        dirent_fill((ext2_dirent_s*)((u8*)entry + used), ctx, rest);
        return DIR_SCAN_DIRTY;
    }

    return 0;
}

i32 ext2_dir_add(fs_inode_s* dir, const char* name, u32 ino, u8 file_type) {
    ext2_fs_s* fs = ext2_fs_of(dir->sb);
    dir_name_ctx_s ctx = { name, (u32)strlen(name), ino, file_type };
    if (ctx.len == 0 || ctx.len > EXT2_NAME_MAX)
        return -(i32)E_NAMELONG;

    i32 ret = dir_scan(dir, add_visit, &ctx);
    if (ret)
        return (ret > 0) ? E_OK : ret;

    u32 index = (u32)(dir->size >> fs->block_log);
    u32 block;
    ret = ext2_bmap(dir, index, 1, &block);
    if (ret != E_OK)
        return ret;

    buf_s* buffer = bread(block);
    if (!buffer)
        return -(i32)E_IO;

    memset(buffer->data, 0, fs->block_size);
    dirent_fill((ext2_dirent_s*)buffer->data, &ctx, (u16)fs->block_size);
    bdirty(buffer);
    brelse(buffer);

    dir->size = (i64)(index + 1u) * fs->block_size;
    return ext2_iwrite(dir);
}

static i32 remove_visit(ext2_dirent_s* entry, ext2_dirent_s* previous, u32 offset, void* context) {
    dir_name_ctx_s* ctx = context;
    if (!name_matches(entry, ctx))
        return 0;

    if (previous)
        previous->rec_len += entry->rec_len;
    else
        entry->inode = 0;

    return DIR_SCAN_DIRTY;
}

i32 ext2_dir_remove(fs_inode_s* dir, const char* name) {
    dir_name_ctx_s ctx = { name, (u32)strlen(name), 0, 0 };
    i32 ret = dir_scan(dir, remove_visit, &ctx);
    if (ret == DIR_SCAN_DIRTY)
        return E_OK;

    return (ret < 0) ? ret : -(i32)E_NOENT;
}

static int name_is_dots(const ext2_dirent_s* entry) {
    if (entry->name_len == 1 && entry->name[0] == '.')
        return 1;

    return entry->name_len == 2 && entry->name[0] == '.' && entry->name[1] == '.';
}

static i32 empty_visit(ext2_dirent_s* entry, ext2_dirent_s* previous, u32 offset, void* context) {
    if (!entry->inode || name_is_dots(entry))
        return 0;

    return DIR_SCAN_FOUND;
}

i32 ext2_dir_is_empty(fs_inode_s* dir) {
    i32 ret = dir_scan(dir, empty_visit, NULL);
    if (ret == DIR_SCAN_FOUND)
        return 0;

    return (ret < 0) ? ret : 1;
}

static i32 parent_visit(ext2_dirent_s* entry, ext2_dirent_s* previous, u32 offset, void* context) {
    if (!entry->inode || entry->name_len != 2 ||
        entry->name[0] != '.' || entry->name[1] != '.')
        return 0;

    entry->inode = *(u32*)context;
    return DIR_SCAN_DIRTY;
}

i32 ext2_dir_set_parent(fs_inode_s* dir, u32 parent) {
    i32 ret = dir_scan(dir, parent_visit, &parent);
    if (ret == DIR_SCAN_DIRTY)
        return E_OK;

    return (ret < 0) ? ret : -(i32)E_NOENT;
}

static u32 dirent_dt(u8 file_type) {
    switch (file_type) {
        case EXT2_FT_DIR:
            return VFS_DT_DIR;
        case EXT2_FT_SYMLINK:
            return VFS_DT_LNK;
        default:
            return VFS_DT_REG;
    }
}

typedef struct dir_iter_ctx_s {
    u32                 cookie; /* Byte offset to resume at */
    fs_readdir_reply_s* reply;  /* Reply under construction */
} dir_iter_ctx_s;

static i32 iter_visit(ext2_dirent_s* entry, ext2_dirent_s* previous, u32 offset, void* context) {
    dir_iter_ctx_s* ctx = context;
    if (offset < ctx->cookie || !entry->inode)
        return 0;

    u32 len = min((u32)entry->name_len, (u32)(VFS_NAME_MAX - 1u));
    ctx->reply->cookie = offset + entry->rec_len;
    ctx->reply->ino    = entry->inode;
    ctx->reply->type   = dirent_dt(entry->file_type);
    memcpy(ctx->reply->name, entry->name, len);
    ctx->reply->name[len] = '\0';
    return DIR_SCAN_FOUND;
}

i32 ext2_readdir(fs_inode_s* dir, u32 cookie, fs_readdir_reply_s* reply) {
    dir_iter_ctx_s ctx = { cookie, reply };
    i32 ret = dir_scan(dir, iter_visit, &ctx);
    if (ret == DIR_SCAN_FOUND)
        return 1;

    return (ret < 0) ? ret : 0;
}
