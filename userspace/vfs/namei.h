#pragma once

#include <userspace/vfs/vfs.h>

#define LOOKUP_FOLLOW    0x1u /* Follow a final symlink        */
#define LOOKUP_DIRECTORY 0x2u /* Final component must be a dir */
#define LOOKUP_PARENT    0x4u /* Stop at the final component   */

/**
 * path_lookup - Resolves @path into a referenced @result.
 * @path: The path to resolve.
 * @flags: The LOOKUP_* resolution flags.
 * @result: Filled with the referenced resolved path.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 path_lookup(const char* path, u32 flags, path_s* result);

/**
 * do_open - Opens @path with VFS_O_* @flags, creating with @mode when asked.
 * @path: The path to open.
 * @flags: The VFS_O_* open flags.
 * @mode: The mode to create with, when VFS_O_CREAT is set.
 * @result: Filled with the new file object on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_open(const char* path, u32 flags, u32 mode, file_s** result);

/**
 * do_mkdir - Creates the directory @path with @mode.
 * @path: The path of the directory to create.
 * @mode: The mode to create with.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_mkdir(const char* path, u32 mode);

/**
 * do_rmdir - Removes the empty directory @path.
 * @path: The path of the directory to remove.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_rmdir(const char* path);

/**
 * do_unlink - Removes the non-directory entry @path.
 * @path: The path of the entry to remove.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_unlink(const char* path);

/**
 * do_link - Hard-links @source to the new entry @target.
 * @source: The path of the existing entry.
 * @target: The path of the new hard link.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_link(const char* source, const char* target);

/**
 * do_symlink - Creates the symlink @target holding @content.
 * @content: The symlink content.
 * @target: The path of the new symlink.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_symlink(const char* content, const char* target);

/**
 * do_rename - Moves @source over @target, replacing a compatible victim.
 * @source: The path of the entry to move.
 * @target: The destination path.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_rename(const char* source, const char* target);

/**
 * do_truncate - Resizes the regular file @path to @length bytes.
 * @path: The path of the regular file to resize.
 * @length: The new size in bytes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_truncate(const char* path, i64 length);

/**
 * do_readlink - Copies up to @size bytes of @path's symlink content into @buffer.
 * @path: The path of the symlink to read.
 * @buffer: The destination buffer.
 * @size: The maximum number of bytes to copy.
 * Returns: The number of bytes copied, or a negative error code on failure.
 */
i32 do_readlink(const char* path, char* buffer, u32 size);

/**
 * do_stat - Fills @stat for @path, honouring VFS_O_NOFOLLOW in @flags.
 * @path: The path to stat.
 * @flags: The VFS_O_* flags, honouring VFS_O_NOFOLLOW.
 * @stat: Filled with @path's attributes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_stat(const char* path, u32 flags, vfs_stat_s* stat);
