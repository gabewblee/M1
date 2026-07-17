#pragma once

#include <userspace/fs/lib/fs.h>

/**
 * fs_super_init - Initializes the superblock table.
 */
void fs_super_init(void);

/**
 * fs_super_mount - Builds a superblock for @driver backed by @source.
 * @driver: The filesystem driver.
 * @source: The backing source, filesystem-defined.
 * @flags: The mount flags.
 * @result: Filled with the new superblock on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fs_super_mount(const fs_driver_s* driver, const char* source, u32 flags, fs_super_s** result);

/**
 * fs_super_find - Returns the mounted superblock with wire handle @id, or NULL.
 * @id: The wire superblock handle.
 * Returns: The mounted superblock, or NULL if not found.
 */
fs_super_s* fs_super_find(u32 id);

/**
 * fs_super_umount - Unpins the root, evicts leftover inodes, and frees @sb.
 * @sb: The superblock to tear down.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fs_super_umount(fs_super_s* sb);

/**
 * fs_super_ino - Returns the next unused node id on @sb.
 * @sb: The superblock to allocate from.
 * Returns: The next unused node id.
 */
u32 fs_super_ino(fs_super_s* sb);
