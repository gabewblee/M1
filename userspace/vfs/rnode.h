#pragma once

#include <userspace/vfs/vfs.h>

/**
 * rnode_init - Initializes the remote node cache.
 */
void rnode_init(void);

/**
 * rnode_get - Returns a referenced node for (@sb, @ino), absorbing one remote
 *             reference granted by the filesystem server.
 * @sb: The owning remote superblock.
 * @ino: The remote node id.
 * @mode: The node's type and mode bits.
 * Returns: The referenced node, or NULL when out of memory.
 */
rnode_s* rnode_get(rsb_s* sb, u32 ino, u32 mode);

/**
 * rnode_hold - Takes an extra local reference on @node.
 * @node: The node to reference.
 * Returns: @node.
 */
rnode_s* rnode_hold(rnode_s* node);

/**
 * rnode_put - Drops a local reference, returning the accumulated remote
 *             references to the filesystem server at zero.
 * @node: The node to drop a reference on.
 */
void rnode_put(rnode_s* node);

/**
 * rnode_stat - Fills @stat with @node's current remote attributes.
 * @node: The node to stat.
 * @stat: Filled with the node's attributes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 rnode_stat(rnode_s* node, vfs_stat_s* stat);

/**
 * rnode_size - Stores @node's current remote size in @size.
 * @node: The node to query.
 * @size: Filled with the node's size in bytes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 rnode_size(rnode_s* node, i64* size);
