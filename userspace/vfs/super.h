#pragma once

#include <userspace/vfs/vfs.h>

/**
 * super_init - Initializes the superblock cache.
 */
void super_init(void);

/**
 * fstype_register - Adds @fstype to the filesystem registry.
 * @fstype: The filesystem type to register.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 fstype_register(fstype_s* fstype);

/**
 * fstype_find - Returns the registered filesystem named @name, or NULL.
 * @name: The filesystem name to look up.
 * Returns: The registered filesystem, or NULL if not found.
 */
fstype_s* fstype_find(const char* name);

/**
 * super_alloc - Allocates a superblock for @fstype, or NULL when out of memory.
 * @fstype: The filesystem type to allocate for.
 * Returns: The new superblock, or NULL when out of memory.
 */
super_block_s* super_alloc(const fstype_s* fstype);

/**
 * super_kill - Runs the filesystem teardown, evicts leftover inodes, and frees @sb.
 * @sb: The superblock to tear down.
 */
void super_kill(super_block_s* sb);

/**
 * super_ino - Returns the next unused inode number on @sb.
 * @sb: The superblock to allocate from.
 * Returns: The next unused inode number.
 */
u32 super_ino(super_block_s* sb);
