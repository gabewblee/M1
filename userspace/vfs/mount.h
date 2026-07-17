#pragma once

#include <userspace/vfs/vfs.h>

/**
 * mount_init - Initializes the mount table and mounts the root filesystem @fstype.
 * @fstype: The name of the root filesystem type.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 mount_init(const char* fstype);

/**
 * mount_lookup - Returns the mount whose mountpoint is (@mount, @dentry), or NULL.
 * @mount: The parent mount to search.
 * @dentry: The mountpoint dentry to search for.
 * Returns: The covering mount, or NULL if none.
 */
mount_s* mount_lookup(mount_s* mount, dentry_s* dentry);

/**
 * mntget - Takes a reference on @mount.
 * @mount: The mount to reference.
 * Returns: @mount.
 */
mount_s* mntget(mount_s* mount);

/**
 * mntput - Drops a reference, tearing the filesystem down at zero.
 * @mount: The mount to drop a reference on.
 */
void mntput(mount_s* mount);

/**
 * do_mount - Mounts @fstype backed by @source on the directory at @target.
 * @source: The backing source, filesystem-defined.
 * @target: The path of the directory to mount on.
 * @fstype: The name of the filesystem type.
 * @flags: The mount flags.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_mount(const char* source, const char* target, const char* fstype, u32 flags);

/**
 * do_umount - Unmounts the mount whose root is at @target.
 * @target: The path of the mount root.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_umount(const char* target);

/**
 * do_statfs - Fills @statfs for the filesystem containing @path.
 * @path: The path to query.
 * @statfs: Filled with the filesystem's statistics.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 do_statfs(const char* path, vfs_statfs_s* statfs);

/**
 * do_sync - Flushes dirty state of every mounted filesystem.
 * Returns: E_OK on success, or the first negative error code encountered.
 */
i32 do_sync(void);

/**
 * path_get - Takes references on both halves of @path.
 * @path: The path to reference.
 */
void path_get(path_s* path);

/**
 * path_put - Drops the references held by @path.
 * @path: The path to drop references on.
 */
void path_put(path_s* path);

/**
 * path_root - Fills @result with a referenced copy of the VFS root.
 * @result: Filled with a referenced copy of the VFS root.
 */
void path_root(path_s* result);
