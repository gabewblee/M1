#pragma once

#include <userspace/vfs/vfs.h>

/**
 * icache_init - Initializes the inode cache.
 */
void icache_init(void);

/**
 * inode_alloc - Allocates a referenced inode on @sb, or NULL when out of memory.
 * @sb: The owning superblock.
 * Returns: The new referenced inode, or NULL when out of memory.
 */
inode_s* inode_alloc(super_block_s* sb);

/**
 * inode_insert - Hashes @inode by (superblock, inode number).
 * @inode: The inode to hash.
 */
void inode_insert(inode_s* inode);

/**
 * iget - Returns a referenced in-core inode for (@sb, @ino), or NULL on miss.
 * @sb: The owning superblock.
 * @ino: The inode number to look up.
 * Returns: A referenced in-core inode, or NULL on miss.
 */
inode_s* iget(super_block_s* sb, u32 ino);

/**
 * ihold - Takes an extra reference on @inode.
 * @inode: The inode to reference.
 * Returns: @inode.
 */
inode_s* ihold(inode_s* inode);

/**
 * iput - Drops a reference, evicting @inode when unreferenced and unlinked.
 * @inode: The inode to drop a reference on.
 */
void iput(inode_s* inode);

/**
 * inode_evict - Unconditionally drops @inode's data and frees it.
 * @inode: The inode to evict.
 */
void inode_evict(inode_s* inode);

/**
 * inode_stat - Fills @stat from @inode's attributes.
 * @inode: The inode to read attributes from.
 * @stat: Filled with @inode's attributes.
 */
void inode_stat(const inode_s* inode, vfs_stat_s* stat);
