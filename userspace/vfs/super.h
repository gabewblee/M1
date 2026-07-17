#pragma once

#include <userspace/vfs/vfs.h>

/**
 * super_init - Initializes the remote superblock cache.
 */
void super_init(void);

/**
 * fstype_find - Returns the filesystem type named @name, or NULL.
 * @name: The filesystem name to look up.
 * Returns: The filesystem type, or NULL if not found.
 */
const fstype_s* fstype_find(const char* name);

/**
 * rsb_mount - Asks @fstype's server to mount @source, wrapping the result.
 * @fstype: The filesystem type to mount.
 * @source: The backing source, filesystem-defined.
 * @flags: The mount flags.
 * @result: Filled with a referenced remote superblock on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 rsb_mount(const fstype_s* fstype, const char* source, u32 flags, rsb_s** result);

/**
 * rsb_get - Takes a reference on @sb.
 * @sb: The remote superblock to reference.
 * Returns: @sb.
 */
rsb_s* rsb_get(rsb_s* sb);

/**
 * rsb_put - Drops a reference, unmounting the remote superblock at zero.
 * @sb: The remote superblock to drop a reference on.
 */
void rsb_put(rsb_s* sb);
