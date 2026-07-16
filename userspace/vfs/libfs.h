#pragma once

#include <userspace/vfs/vfs.h>

extern const file_ops_s simple_dir_ops; /* dcache-backed directory file ops */

/**
 * simple_lookup - Caches a negative dentry; directory content lives in the dcache.
 * @dir: The directory inode being searched.
 * @dentry: The dentry to instantiate, left negative.
 * Returns: NULL always.
 */
dentry_s* simple_lookup(inode_s* dir, dentry_s* dentry);

/**
 * simple_empty - Checks that @dentry has no positive children.
 * @dentry: The directory dentry to check.
 * Returns: 1 if @dentry has no positive children, 0 otherwise.
 */
int simple_empty(dentry_s* dentry);

/**
 * simple_unlink - Drops one link and the dcache pin of @dentry.
 * @dir: The parent directory inode.
 * @dentry: The dentry to unlink.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 simple_unlink(inode_s* dir, dentry_s* dentry);

/**
 * simple_rmdir - Removes the empty directory @dentry, fixing link counts.
 * @dir: The parent directory inode.
 * @dentry: The directory dentry to remove.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 simple_rmdir(inode_s* dir, dentry_s* dentry);

/**
 * simple_rename - Moves @from over @to, unlinking a compatible victim.
 * @source: The source directory inode.
 * @from: The source dentry.
 * @target: The target directory inode.
 * @to: The target dentry, unlinked if it is a compatible victim.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 simple_rename(inode_s* source, dentry_s* from, inode_s* target, dentry_s* to);

/**
 * generic_seek - Standard SET/CUR/END repositioning against the inode size.
 * @file: The file to reposition.
 * @offset: The seek offset.
 * @whence: The VFS_SEEK_* origin the offset is relative to.
 * @result: Filled with the new file position on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 generic_seek(file_s* file, i64 offset, u32 whence, i64* result);

/**
 * dcache_readdir - Emits ".", "..", then the positive children of the directory.
 * @file: The open directory file.
 * @entry: Filled with the next directory entry.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 dcache_readdir(file_s* file, vfs_dirent_s* entry);

/**
 * dcache_dir_seek - Repositions a directory stream, invalidating its cursor.
 * @file: The open directory file.
 * @offset: The seek offset.
 * @whence: The VFS_SEEK_* origin the offset is relative to.
 * @result: Filled with the new file position on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 dcache_dir_seek(file_s* file, i64 offset, u32 whence, i64* result);

/**
 * dcache_dir_release - Drops the readdir cursor reference.
 * @inode: The directory inode.
 * @file: The file being released.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 dcache_dir_release(inode_s* inode, file_s* file);
