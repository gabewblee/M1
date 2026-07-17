#pragma once

#include <userspace/vfs/vfs.h>

typedef struct dcache_stats_s {
    u32 hits;      /* Positive and negative cache hits    */
    u32 misses;    /* Lookups that fell through to the fs */
    u32 allocated; /* Dentries created                    */
    u32 pruned;    /* Dentries reclaimed from the LRU     */
} dcache_stats_s;

/**
 * dcache_init - Initializes the dentry cache and registers its shrinker.
 */
void dcache_init(void);

/**
 * d_alloc - Allocates a dentry named @name under @parent, self-parented when @parent is NULL.
 * @parent: The parent dentry, or NULL to allocate a self-parented root.
 * @name: The component name to allocate.
 * Returns: The new dentry, or NULL on failure.
 */
dentry_s* d_alloc(dentry_s* parent, const qstr_s* name);

/**
 * d_instantiate - Binds @node to @dentry, consuming one node reference.
 * @dentry: The dentry to bind.
 * @node: The node to bind, its reference consumed.
 */
void d_instantiate(dentry_s* dentry, rnode_s* node);

/**
 * d_rehash - Inserts @dentry into the dentry hash.
 * @dentry: The dentry to hash.
 */
void d_rehash(dentry_s* dentry);

/**
 * d_add - Binds @node to @dentry and hashes it.
 * @dentry: The dentry to bind and hash.
 * @node: The node to bind, its reference consumed.
 */
void d_add(dentry_s* dentry, rnode_s* node);

/**
 * d_drop - Removes @dentry from the dentry hash.
 * @dentry: The dentry to unhash.
 */
void d_drop(dentry_s* dentry);

/**
 * d_lookup - Returns a referenced child of @parent named @name, or NULL on miss.
 * @parent: The parent dentry to search.
 * @name: The component name to look up.
 * Returns: A referenced child dentry, or NULL on miss.
 */
dentry_s* d_lookup(dentry_s* parent, const qstr_s* name);

/**
 * d_delete - Turns an unlinked @dentry negative, or unhashes it when still shared.
 * @dentry: The unlinked dentry to delete.
 */
void d_delete(dentry_s* dentry);

/**
 * d_move - Moves @dentry to @target's parent and name, unhashing @target.
 * @dentry: The dentry to move.
 * @target: The dentry whose parent and name are taken, then unhashed.
 */
void d_move(dentry_s* dentry, dentry_s* target);

/**
 * d_is_ancestor - Checks whether @dentry is @child or one of its ancestors.
 * @dentry: The candidate ancestor dentry.
 * @child: The dentry to check.
 * Returns: 1 if @dentry is @child or an ancestor of @child, 0 otherwise.
 */
int d_is_ancestor(dentry_s* dentry, dentry_s* child);

/**
 * dget - Takes a reference on @dentry, reviving it from the LRU if needed.
 * @dentry: The dentry to reference.
 * Returns: @dentry.
 */
dentry_s* dget(dentry_s* dentry);

/**
 * dput - Drops a reference, parking or killing @dentry at zero.
 * @dentry: The dentry to drop a reference on.
 */
void dput(dentry_s* dentry);

/**
 * dcache_shrink - Reclaims up to @count unreferenced dentries from the LRU.
 * @count: The maximum number of dentries to reclaim.
 * Returns: The number of dentries reclaimed.
 */
u32 dcache_shrink(u32 count);

/**
 * dcache_prune_sb - Reclaims every unreferenced dentry belonging to @sb.
 * @sb: The remote superblock whose dentries are reclaimed.
 */
void dcache_prune_sb(const rsb_s* sb);

/**
 * dcache_stats - Returns the dentry cache counters.
 * Returns: The dentry cache counters.
 */
const dcache_stats_s* dcache_stats(void);
