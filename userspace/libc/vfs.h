#pragma once

#include <uapi/types.h>
#include <uapi/vfs.h>

/**
 * vfs_client_init - Selects the endpoint used for VFS server calls.
 * @ep: The VFS server endpoint capability pointer.
 */
void vfs_client_init(u32 ep);

/**
 * vfs_open - Opens @path with VFS_O_* @flags, creating with @mode when asked.
 * @path: The path to open.
 * @flags: The VFS_O_* open flags.
 * @mode: The mode to create with, when VFS_O_CREAT is set.
 * Returns: The new descriptor on success, or a negative error code on failure.
 */
i32 vfs_open(const char* path, u32 flags, u32 mode);

/**
 * vfs_close - Closes the descriptor @fd.
 * @fd: The descriptor to close.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_close(i32 fd);

/**
 * vfs_read - Reads up to @count bytes from @fd into @buffer.
 * @fd: The descriptor to read from.
 * @buffer: The destination buffer.
 * @count: The maximum number of bytes to read.
 * Returns: The number of bytes read, or a negative error code on failure.
 */
i32 vfs_read(i32 fd, void* buffer, u32 count);

/**
 * vfs_write - Writes @count bytes from @buffer to @fd.
 * @fd: The descriptor to write to.
 * @buffer: The source buffer.
 * @count: The number of bytes to write.
 * Returns: The number of bytes written, or a negative error code on failure.
 */
i32 vfs_write(i32 fd, const void* buffer, u32 count);

/**
 * vfs_seek - Repositions @fd, storing the new offset in @result.
 * @fd: The descriptor to reposition.
 * @offset: The seek offset.
 * @whence: The VFS_SEEK_* origin the offset is relative to.
 * @result: Filled with the new file position on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_seek(i32 fd, i64 offset, u32 whence, i64* result);

/**
 * vfs_stat - Fills @stat for @path, honouring VFS_O_NOFOLLOW in @flags.
 * @path: The path to stat.
 * @flags: The VFS_O_* flags, honouring VFS_O_NOFOLLOW.
 * @stat: Filled with @path's attributes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_stat(const char* path, u32 flags, vfs_stat_s* stat);

/**
 * vfs_fstat - Fills @stat for the descriptor @fd.
 * @fd: The descriptor to stat.
 * @stat: Filled with the descriptor's attributes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_fstat(i32 fd, vfs_stat_s* stat);

/**
 * vfs_mkdir - Creates the directory @path with @mode.
 * @path: The path of the directory to create.
 * @mode: The mode to create with.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_mkdir(const char* path, u32 mode);

/**
 * vfs_rmdir - Removes the empty directory @path.
 * @path: The path of the directory to remove.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_rmdir(const char* path);

/**
 * vfs_unlink - Removes the non-directory entry @path.
 * @path: The path of the entry to remove.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_unlink(const char* path);

/**
 * vfs_link - Hard-links @source to the new entry @target.
 * @source: The path of the existing entry.
 * @target: The path of the new hard link.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_link(const char* source, const char* target);

/**
 * vfs_symlink - Creates the symlink @target holding @content.
 * @content: The symlink content.
 * @target: The path of the new symlink.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_symlink(const char* content, const char* target);

/**
 * vfs_readlink - Copies up to @size bytes of @path's symlink content.
 * @path: The path of the symlink to read.
 * @buffer: The destination buffer.
 * @size: The maximum number of bytes to copy.
 * Returns: The number of bytes copied, or a negative error code on failure.
 */
i32 vfs_readlink(const char* path, char* buffer, u32 size);

/**
 * vfs_rename - Moves @source over @target, replacing a compatible victim.
 * @source: The path of the entry to move.
 * @target: The destination path.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_rename(const char* source, const char* target);

/**
 * vfs_readdir - Emits the next directory entry of @fd into @entry.
 * @fd: The open directory descriptor.
 * @entry: Filled with the next directory entry.
 * Returns: 1 when an entry was emitted, 0 at the end of the directory, or a
 *          negative error code on failure.
 */
i32 vfs_readdir(i32 fd, vfs_dirent_s* entry);

/**
 * vfs_truncate - Resizes the regular file @path to @length bytes.
 * @path: The path of the regular file to resize.
 * @length: The new size in bytes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_truncate(const char* path, i64 length);

/**
 * vfs_mount - Mounts @fstype backed by @source on the directory at @target.
 * @source: The backing source, filesystem-defined.
 * @target: The path of the directory to mount on.
 * @fstype: The name of the filesystem type.
 * @flags: The mount flags.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_mount(const char* source, const char* target, const char* fstype, u32 flags);

/**
 * vfs_umount - Unmounts the mount whose root is at @target.
 * @target: The path of the mount root.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_umount(const char* target);

/**
 * vfs_statfs - Fills @statfs for the filesystem containing @path.
 * @path: The path to query.
 * @statfs: Filled with the filesystem's statistics.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_statfs(const char* path, vfs_statfs_s* statfs);

/**
 * vfs_sync - Flushes dirty state of every mounted filesystem to disk.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 vfs_sync(void);
