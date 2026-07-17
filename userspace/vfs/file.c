#include <userspace/libc/heap.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>
#include <userspace/vfs/dcache.h>
#include <userspace/vfs/file.h>
#include <userspace/vfs/fsclient.h>
#include <userspace/vfs/mount.h>
#include <userspace/vfs/namei.h>
#include <userspace/vfs/rnode.h>

static cache_s file_object_cache;
static file_s* fd_tables[VFS_CLIENT_MAX][VFS_FD_MAX];

void file_init(void) {
    cache_init(&file_object_cache, "file", sizeof(file_s));
}

i32 file_open(path_s* path, u32 flags, file_s** result) {
    file_s* file = cache_alloc(&file_object_cache);
    if (!file) {
        path_put(path);
        return -(i32)E_NOMEM;
    }

    file->count = 1;
    file->flags = flags;
    file->pos   = 0;
    file->path  = *path;

    path->mount  = NULL;
    path->dentry = NULL;

    *result = file;
    return E_OK;
}

file_s* file_get(file_s* file) {
    file->count++;
    return file;
}

void file_put(file_s* file) {
    if (--file->count > 0)
        return;

    path_put(&file->path);
    cache_free(&file_object_cache, file);
}

static rnode_s* file_node(const file_s* file) {
    return file->path.dentry->node;
}

static int readable(const file_s* file) {
    return (file->flags & VFS_O_ACCMODE) != VFS_O_WRONLY;
}

static int writable(const file_s* file) {
    return (file->flags & VFS_O_ACCMODE) != VFS_O_RDONLY;
}

i32 file_read(file_s* file, void* buffer, u32 count) {
    rnode_s* node = file_node(file);
    if (!readable(file))
        return -(i32)E_BADF;

    if (VFS_ISDIR(node->mode))
        return -(i32)E_ISDIR;

    u32 done = 0;
    while (done < count) {
        u32 chunk = min(count - done, (u32)FS_READ_MAX);
        i32 ret = fsc_read(node->sb, node->ino, (u8*)buffer + done, chunk, file->pos);
        if (ret < 0)
            return done ? (i32)done : ret;

        done += (u32)ret;
        file->pos += ret;
        if ((u32)ret < chunk)
            break;
    }

    return (i32)done;
}

i32 file_write(file_s* file, const void* buffer, u32 count) {
    rnode_s* node = file_node(file);
    if (!writable(file))
        return -(i32)E_BADF;

    if (VFS_ISDIR(node->mode))
        return -(i32)E_ISDIR;

    u32 append = (file->flags & VFS_O_APPEND) != 0;
    u32 done   = 0;
    while (done < count) {
        u32 chunk    = min(count - done, (u32)FS_WRITE_MAX);
        i64 position = file->pos;
        i32 ret = fsc_write(node->sb, node->ino, (const u8*)buffer + done, chunk,
                            file->pos, append, &position);
        if (ret <= 0)
            return done ? (i32)done : ret;

        done += (u32)ret;
        file->pos = position;
        if ((u32)ret < chunk)
            break;
    }

    return (i32)done;
}

i32 file_seek(file_s* file, i64 offset, u32 whence, i64* result) {
    i64 base;
    switch (whence) {
        case VFS_SEEK_SET:
            base = 0;
            break;
        case VFS_SEEK_CUR:
            base = file->pos;
            break;
        case VFS_SEEK_END: {
            i32 ret = rnode_size(file_node(file), &base);
            if (ret != E_OK)
                return ret;
            break;
        }
        default:
            return -(i32)E_INVAL;
    }

    if (base + offset < 0)
        return -(i32)E_INVAL;

    file->pos = base + offset;
    *result   = file->pos;
    return E_OK;
}

i32 file_readdir(file_s* file, vfs_dirent_s* entry) {
    rnode_s* node = file_node(file);
    if (!VFS_ISDIR(node->mode))
        return -(i32)E_NOTDIR;

    fs_readdir_reply_s reply;
    i32 ret = fsc_readdir(node->sb, node->ino, (u32)file->pos, &reply);
    if (ret <= 0)
        return ret;

    entry->ino  = reply.ino;
    entry->type = reply.type;
    strlcpy(entry->name, reply.name, VFS_NAME_MAX);
    file->pos = reply.cookie;
    return 1;
}

static file_s** fd_slot(u32 client, i32 fd) {
    if (client >= VFS_CLIENT_MAX || fd < 0 || fd >= (i32)VFS_FD_MAX)
        return NULL;

    return &fd_tables[client][fd];
}

static file_s* fd_file(u32 client, i32 fd) {
    file_s** slot = fd_slot(client, fd);
    return slot ? *slot : NULL;
}

i32 fd_open(u32 client, const char* path, u32 flags, u32 mode) {
    if (client >= VFS_CLIENT_MAX)
        return -(i32)E_PERM;

    i32 fd = -1;
    for (u32 i = 0; i < VFS_FD_MAX; i++) {
        if (!fd_tables[client][i]) {
            fd = (i32)i;
            break;
        }
    }

    if (fd < 0)
        return -(i32)E_MFILE;

    file_s* file;
    i32 ret = do_open(path, flags, mode, &file);
    if (ret != E_OK)
        return ret;

    fd_tables[client][fd] = file;
    return fd;
}

i32 fd_close(u32 client, i32 fd) {
    file_s** slot = fd_slot(client, fd);
    if (!slot || !*slot)
        return -(i32)E_BADF;

    file_put(*slot);
    *slot = NULL;
    return E_OK;
}

i32 fd_read(u32 client, i32 fd, void* buffer, u32 count) {
    file_s* file = fd_file(client, fd);
    return file ? file_read(file, buffer, count) : -(i32)E_BADF;
}

i32 fd_write(u32 client, i32 fd, const void* buffer, u32 count) {
    file_s* file = fd_file(client, fd);
    return file ? file_write(file, buffer, count) : -(i32)E_BADF;
}

i32 fd_seek(u32 client, i32 fd, i64 offset, u32 whence, i64* result) {
    file_s* file = fd_file(client, fd);
    return file ? file_seek(file, offset, whence, result) : -(i32)E_BADF;
}

i32 fd_stat(u32 client, i32 fd, vfs_stat_s* stat) {
    file_s* file = fd_file(client, fd);
    return file ? rnode_stat(file_node(file), stat) : -(i32)E_BADF;
}

i32 fd_readdir(u32 client, i32 fd, vfs_dirent_s* entry) {
    file_s* file = fd_file(client, fd);
    return file ? file_readdir(file, entry) : -(i32)E_BADF;
}
