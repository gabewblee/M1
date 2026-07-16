#pragma once

#include <userspace/vfs/vfs.h>

/**
 * file_init - Initializes the file object cache.
 */
void file_init(void);

/**
 * file_open - Wraps the resolved @path in a file object, consuming its references.
 * @path: The resolved path, its references consumed.
 * @flags: The VFS_O_* open flags.
 * @result: Filled with the new file object on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 file_open(path_s* path, u32 flags, file_s** result);

/**
 * file_get - Takes a reference on @file.
 * @file: The file to reference.
 * Returns: @file.
 */
file_s* file_get(file_s* file);

/**
 * file_put - Drops a reference, releasing @file at zero.
 * @file: The file to drop a reference on.
 */
void file_put(file_s* file);

/**
 * vfs_read - Reads up to @count bytes into @buffer at *@position.
 * @file: The file to read from.
 * @buffer: The destination buffer.
 * @count: The maximum number of bytes to read.
 * @position: The file position, advanced by the number of bytes read.
 * Returns: The number of bytes read, or a negative error code on failure.
 */
i32 vfs_read(file_s* file, void* buffer, u32 count, i64* position);

/**
 * vfs_write - Writes up to @count bytes from @buffer at *@position.
 * @file: The file to write to.
 * @buffer: The source buffer.
 * @count: The maximum number of bytes to write.
 * @position: The file position, advanced by the number of bytes written.
 * Returns: The number of bytes written, or a negative error code on failure.
 */
i32 vfs_write(file_s* file, const void* buffer, u32 count, i64* position);

/**
 * vfs_seek - Repositions @file, storing the new offset in @result.
 * @file: The file to reposition.
 * @offset: The seek offset.
 * @whence: The VFS_SEEK_* origin the offset is relative to.
 * @result: Filled with the new file position on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_seek(file_s* file, i64 offset, u32 whence, i64* result);

/**
 * vfs_readdir - Emits the next directory entry of @file into @entry.
 * @file: The open directory file.
 * @entry: Filled with the next directory entry.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_readdir(file_s* file, vfs_dirent_s* entry);

/**
 * fd_open - Opens @path for @client, returning a descriptor or a negative error.
 * @client: The client id opening the path.
 * @path: The path to open.
 * @flags: The VFS_O_* open flags.
 * @mode: The mode to create with, when VFS_O_CREAT is set.
 * Returns: The new descriptor on success, or a negative error code on failure.
 */
i32 fd_open(u32 client, const char* path, u32 flags, u32 mode);

/**
 * fd_close - Closes @client's descriptor @fd.
 * @client: The owning client id.
 * @fd: The descriptor to close.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fd_close(u32 client, i32 fd);

/**
 * fd_read - Reads up to @count bytes from @client's descriptor @fd.
 * @client: The owning client id.
 * @fd: The descriptor to read from.
 * @buffer: The destination buffer.
 * @count: The maximum number of bytes to read.
 * Returns: The number of bytes read, or a negative error code on failure.
 */
i32 fd_read(u32 client, i32 fd, void* buffer, u32 count);

/**
 * fd_write - Writes up to @count bytes to @client's descriptor @fd.
 * @client: The owning client id.
 * @fd: The descriptor to write to.
 * @buffer: The source buffer.
 * @count: The maximum number of bytes to write.
 * Returns: The number of bytes written, or a negative error code on failure.
 */
i32 fd_write(u32 client, i32 fd, const void* buffer, u32 count);

/**
 * fd_seek - Repositions @client's descriptor @fd.
 * @client: The owning client id.
 * @fd: The descriptor to reposition.
 * @offset: The seek offset.
 * @whence: The VFS_SEEK_* origin the offset is relative to.
 * @result: Filled with the new file position on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fd_seek(u32 client, i32 fd, i64 offset, u32 whence, i64* result);

/**
 * fd_stat - Fills @stat for @client's descriptor @fd.
 * @client: The owning client id.
 * @fd: The descriptor to stat.
 * @stat: Filled with the descriptor's attributes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fd_stat(u32 client, i32 fd, vfs_stat_s* stat);

/**
 * fd_readdir - Emits the next directory entry of @client's descriptor @fd.
 * @client: The owning client id.
 * @fd: The open directory descriptor.
 * @entry: Filled with the next directory entry.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fd_readdir(u32 client, i32 fd, vfs_dirent_s* entry);
