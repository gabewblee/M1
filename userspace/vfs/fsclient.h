#pragma once

#include <userspace/vfs/vfs.h>

/**
 * fsc_mount - Asks the server at @ep to mount @source, filling @reply.
 * @ep: The filesystem server endpoint pointer.
 * @source: The backing source, filesystem-defined.
 * @flags: The mount flags.
 * @reply: Filled with the superblock description on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_mount(u32 ep, const char* source, u32 flags, fs_mount_reply_s* reply);

/**
 * fsc_umount - Releases the remote superblock behind @sb.
 * @sb: The remote superblock.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_umount(rsb_s* sb);

/**
 * fsc_sync - Flushes dirty state of the remote superblock behind @sb.
 * @sb: The remote superblock.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_sync(rsb_s* sb);

/**
 * fsc_statfs - Fills @statfs for the remote superblock behind @sb.
 * @sb: The remote superblock.
 * @statfs: Filled with the filesystem's statistics.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_statfs(rsb_s* sb, vfs_statfs_s* statfs);

/**
 * fsc_lookup - Resolves the component @name of length @len under @dir.
 * @sb: The remote superblock.
 * @dir: The parent directory node id.
 * @name: The component bytes, not necessarily NUL-terminated.
 * @len: The component length.
 * @entry: Filled with the resolved entry on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_lookup(rsb_s* sb, u32 dir, const char* name, u32 len, fs_entry_reply_s* entry);

/**
 * fsc_forget - Returns @count node references of @ino to the server.
 * @sb: The remote superblock.
 * @ino: The remote node id.
 * @count: The number of references to return.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_forget(rsb_s* sb, u32 ino, u32 count);

/**
 * fsc_getattr - Fills @entry with the current attributes of @ino.
 * @sb: The remote superblock.
 * @ino: The remote node id.
 * @entry: Filled with the node attributes on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_getattr(rsb_s* sb, u32 ino, fs_entry_reply_s* entry);

/**
 * fsc_create - Creates the regular file @name of length @len under @dir.
 * @sb: The remote superblock.
 * @dir: The parent directory node id.
 * @name: The component bytes, not necessarily NUL-terminated.
 * @len: The component length.
 * @mode: The mode to create with.
 * @entry: Filled with the new entry on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_create(rsb_s* sb, u32 dir, const char* name, u32 len, u32 mode, fs_entry_reply_s* entry);

/**
 * fsc_mkdir - Creates the directory @name of length @len under @dir.
 * @sb: The remote superblock.
 * @dir: The parent directory node id.
 * @name: The component bytes, not necessarily NUL-terminated.
 * @len: The component length.
 * @mode: The mode to create with.
 * @entry: Filled with the new entry on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_mkdir(rsb_s* sb, u32 dir, const char* name, u32 len, u32 mode, fs_entry_reply_s* entry);

/**
 * fsc_rmdir - Removes the directory @name of length @len under @dir.
 * @sb: The remote superblock.
 * @dir: The parent directory node id.
 * @name: The component bytes, not necessarily NUL-terminated.
 * @len: The component length.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_rmdir(rsb_s* sb, u32 dir, const char* name, u32 len);

/**
 * fsc_unlink - Removes the link @name of length @len under @dir.
 * @sb: The remote superblock.
 * @dir: The parent directory node id.
 * @name: The component bytes, not necessarily NUL-terminated.
 * @len: The component length.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_unlink(rsb_s* sb, u32 dir, const char* name, u32 len);

/**
 * fsc_link - Hard-links @ino as @name of length @len under @dir.
 * @sb: The remote superblock.
 * @ino: The existing node id.
 * @dir: The parent directory node id.
 * @name: The component bytes, not necessarily NUL-terminated.
 * @len: The component length.
 * @entry: Filled with the new entry on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_link(rsb_s* sb, u32 ino, u32 dir, const char* name, u32 len, fs_entry_reply_s* entry);

/**
 * fsc_symlink - Creates the symlink @name of length @len holding @target.
 * @sb: The remote superblock.
 * @dir: The parent directory node id.
 * @name: The component bytes, not necessarily NUL-terminated.
 * @len: The component length.
 * @target: The NUL-terminated symlink content.
 * @entry: Filled with the new entry on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_symlink(rsb_s* sb, u32 dir, const char* name, u32 len, const char* target,
                fs_entry_reply_s* entry);

/**
 * fsc_readlink - Copies up to @size bytes of @ino's symlink content.
 * @sb: The remote superblock.
 * @ino: The remote node id.
 * @buffer: The destination buffer.
 * @size: The maximum number of bytes to copy.
 * Returns: The number of bytes copied, or a negative error code on failure.
 */
i32 fsc_readlink(rsb_s* sb, u32 ino, char* buffer, u32 size);

/**
 * fsc_rename - Moves (@from_dir, @from) over (@to_dir, @to).
 * @sb: The remote superblock.
 * @from_dir: The source directory node id.
 * @from: The source component.
 * @to_dir: The destination directory node id.
 * @to: The destination component.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_rename(rsb_s* sb, u32 from_dir, const qstr_s* from, u32 to_dir, const qstr_s* to);

/**
 * fsc_truncate - Resizes the regular file @ino to @length bytes.
 * @sb: The remote superblock.
 * @ino: The remote node id.
 * @length: The new size in bytes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fsc_truncate(rsb_s* sb, u32 ino, i64 length);

/**
 * fsc_read - Reads up to @count bytes of @ino at @offset into @buffer.
 * @sb: The remote superblock.
 * @ino: The remote node id.
 * @buffer: The destination buffer.
 * @count: The maximum number of bytes to read.
 * @offset: The file offset to read at.
 * Returns: The number of bytes read, or a negative error code on failure.
 */
i32 fsc_read(rsb_s* sb, u32 ino, void* buffer, u32 count, i64 offset);

/**
 * fsc_write - Writes up to @count bytes from @buffer to @ino at @offset.
 * @sb: The remote superblock.
 * @ino: The remote node id.
 * @buffer: The source buffer.
 * @count: The maximum number of bytes to write.
 * @offset: The file offset to write at, ignored when @append is set.
 * @append: Whether to write at the end of the file.
 * @position: Filled with the file position after the write.
 * Returns: The number of bytes written, or a negative error code on failure.
 */
i32 fsc_write(rsb_s* sb, u32 ino, const void* buffer, u32 count, i64 offset, u32 append,
              i64* position);

/**
 * fsc_readdir - Fetches the directory entry of @ino at @cookie.
 * @sb: The remote superblock.
 * @ino: The directory node id.
 * @cookie: The resume cookie, 0 at the start of the directory.
 * @reply: Filled with the entry and the next cookie on success.
 * Returns: 1 when an entry was emitted, 0 at the end of the directory, or a
 *          negative error code on failure.
 */
i32 fsc_readdir(rsb_s* sb, u32 ino, u32 cookie, fs_readdir_reply_s* reply);
