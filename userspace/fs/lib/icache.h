#pragma once

#include <userspace/fs/lib/fs.h>

/**
 * fs_icache_init - Initializes the inode cache.
 */
void fs_icache_init(void);

/**
 * fs_inode_new - Allocates a referenced inode on @sb through the driver.
 * @sb: The owning superblock.
 * Returns: The new referenced inode, or NULL when out of memory.
 */
fs_inode_s* fs_inode_new(fs_super_s* sb);

/**
 * fs_inode_hash - Hashes @inode by (superblock, node id).
 * @inode: The inode to hash.
 */
void fs_inode_hash(fs_inode_s* inode);

/**
 * fs_ilookup - Returns the in-core inode for (@sb, @ino) without referencing
 *              it, or NULL on miss.
 * @sb: The owning superblock.
 * @ino: The node id to look up.
 * Returns: The in-core inode, or NULL on miss.
 */
fs_inode_s* fs_ilookup(fs_super_s* sb, u32 ino);

/**
 * fs_iget - Returns a referenced in-core inode for (@sb, @ino), or NULL on miss.
 * @sb: The owning superblock.
 * @ino: The node id to look up.
 * Returns: A referenced in-core inode, or NULL on miss.
 */
fs_inode_s* fs_iget(fs_super_s* sb, u32 ino);

/**
 * fs_ihold - Takes an extra reference on @inode.
 * @inode: The inode to reference.
 * Returns: @inode.
 */
fs_inode_s* fs_ihold(fs_inode_s* inode);

/**
 * fs_iput - Drops @count references, evicting @inode when it becomes both
 *           unreferenced and evictable.
 * @inode: The inode to drop references on.
 * @count: The number of references to drop.
 */
void fs_iput(fs_inode_s* inode, u32 count);

/**
 * fs_inode_evict - Unconditionally drops @inode's state and frees it.
 * @inode: The inode to evict.
 */
void fs_inode_evict(fs_inode_s* inode);
