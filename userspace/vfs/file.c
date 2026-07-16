#include <userspace/vfs/file.h>
#include <userspace/vfs/heap.h>
#include <userspace/vfs/inode.h>
#include <userspace/vfs/libfs.h>
#include <userspace/vfs/mount.h>
#include <userspace/vfs/namei.h>

typedef struct fdtable_s {
    u32     bitmap;             /* Allocated descriptor bits */
    file_s* files[VFS_FD_MAX];  /* Open file table           */
} fdtable_s;

static cache_s   file_object_cache;
static fdtable_s client_file_tables[VFS_CLIENT_MAX];

static inode_s* file_inode(file_s* file) {
    return file->path.dentry->inode;
}

i32 file_open(path_s* path, u32 flags, file_s** result) {
    file_s* file = cache_alloc(&file_object_cache);
    if (!file) {
        path_put(path);
        return -(i32)E_NOMEM;
    }

    file->count   = 1;
    file->flags   = flags;
    file->pos     = 0;
    file->path    = *path;
    file->ops     = path->dentry->inode->fops;
    file->private = NULL;

    if (file->ops && file->ops->open) {
        i32 ret = file->ops->open(file_inode(file), file);
        if (ret != E_OK) {
            path_put(&file->path);
            cache_free(&file_object_cache, file);
            return ret;
        }
    }

    *result = file;
    return E_OK;
}

file_s* file_get(file_s* file) {
    file->count++;
    return file;
}

void file_put(file_s* file) {
    if (!file || --file->count)
        return;

    if (file->ops && file->ops->release)
        file->ops->release(file_inode(file), file);

    path_put(&file->path);
    cache_free(&file_object_cache, file);
}

i32 vfs_read(file_s* file, void* buffer, u32 count, i64* position) {
    if (VFS_ISDIR(file_inode(file)->mode))
        return -(i32)E_ISDIR;

    if ((file->flags & VFS_O_ACCMODE) == VFS_O_WRONLY)
        return -(i32)E_BADF;

    if (!file->ops || !file->ops->read)
        return -(i32)E_NOSYS;

    if (count == 0)
        return 0;

    return file->ops->read(file, buffer, count, position);
}

i32 vfs_write(file_s* file, const void* buffer, u32 count, i64* position) {
    inode_s* inode = file_inode(file);
    if (VFS_ISDIR(inode->mode))
        return -(i32)E_ISDIR;

    if ((file->flags & VFS_O_ACCMODE) == VFS_O_RDONLY)
        return -(i32)E_BADF;

    if (!file->ops || !file->ops->write)
        return -(i32)E_NOSYS;

    if (count == 0)
        return 0;

    if (file->flags & VFS_O_APPEND)
        *position = inode->size;

    return file->ops->write(file, buffer, count, position);
}

i32 vfs_seek(file_s* file, i64 offset, u32 whence, i64* result) {
    if (file->ops && file->ops->llseek)
        return file->ops->llseek(file, offset, whence, result);

    return generic_seek(file, offset, whence, result);
}

i32 vfs_readdir(file_s* file, vfs_dirent_s* entry) {
    if (!VFS_ISDIR(file_inode(file)->mode))
        return -(i32)E_NOTDIR;

    if (!file->ops || !file->ops->readdir)
        return -(i32)E_NOSYS;

    return file->ops->readdir(file, entry);
}

static fdtable_s* fd_table(u32 client) {
    return client < VFS_CLIENT_MAX ? &client_file_tables[client] : NULL;
}

static file_s* fd_file(fdtable_s* table, i32 fd) {
    if (!table || fd < 0 || fd >= (i32)VFS_FD_MAX)
        return NULL;

    if (!(table->bitmap & (1u << fd)))
        return NULL;

    return table->files[fd];
}

i32 fd_open(u32 client, const char* path, u32 flags, u32 mode) {
    fdtable_s* table = fd_table(client);
    if (!table)
        return -(i32)E_PERM;

    if (table->bitmap == 0xFFFFFFFFu)
        return -(i32)E_MFILE;

    file_s* file;
    i32 ret = do_open(path, flags, mode, &file);
    if (ret != E_OK)
        return ret;

    i32 slot = (i32)__builtin_ctz(~table->bitmap);
    table->bitmap     |= 1u << slot;
    table->files[slot] = file;
    return slot;
}

i32 fd_close(u32 client, i32 fd) {
    fdtable_s* table = fd_table(client);
    file_s* file = fd_file(table, fd);
    if (!file)
        return -(i32)E_BADF;

    table->bitmap   &= ~(1u << fd);
    table->files[fd] = NULL;
    file_put(file);
    return E_OK;
}

i32 fd_read(u32 client, i32 fd, void* buffer, u32 count) {
    file_s* file = fd_file(fd_table(client), fd);
    if (!file)
        return -(i32)E_BADF;

    return vfs_read(file, buffer, count, &file->pos);
}

i32 fd_write(u32 client, i32 fd, const void* buffer, u32 count) {
    file_s* file = fd_file(fd_table(client), fd);
    if (!file)
        return -(i32)E_BADF;

    return vfs_write(file, buffer, count, &file->pos);
}

i32 fd_seek(u32 client, i32 fd, i64 offset, u32 whence, i64* result) {
    file_s* file = fd_file(fd_table(client), fd);
    if (!file)
        return -(i32)E_BADF;

    return vfs_seek(file, offset, whence, result);
}

i32 fd_stat(u32 client, i32 fd, vfs_stat_s* stat) {
    file_s* file = fd_file(fd_table(client), fd);
    if (!file)
        return -(i32)E_BADF;

    inode_stat(file_inode(file), stat);
    return E_OK;
}

i32 fd_readdir(u32 client, i32 fd, vfs_dirent_s* entry) {
    file_s* file = fd_file(fd_table(client), fd);
    if (!file)
        return -(i32)E_BADF;

    return vfs_readdir(file, entry);
}

void file_init(void) {
    cache_init(&file_object_cache, "file", sizeof(file_s));
}
