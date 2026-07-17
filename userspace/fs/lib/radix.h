#pragma once

#include <uapi/types.h>

#define RADIX_BITS  6u  /* Index bits consumed per level */
#define RADIX_SLOTS 64u /* Slots per node                */

typedef struct radix_s {
    void* root;   /* Top node, NULL when empty */
    u32   height; /* Tree levels, 0 when empty */
} radix_s;

typedef void (*radix_release_f)(void* item);

/**
 * radix_init - Initializes the shared radix node cache.
 */
void radix_init(void);

/**
 * radix_tree_init - Initializes @tree to the empty state.
 * @tree: The radix tree to initialize.
 */
void radix_tree_init(radix_s* tree);

/**
 * radix_insert - Inserts @item at @index, growing the tree as needed.
 * @tree: The radix tree to insert into.
 * @index: The index to insert at.
 * @item: The item to insert.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 radix_insert(radix_s* tree, u32 index, void* item);

/**
 * radix_lookup - Returns the item at @index, or NULL when absent.
 * @tree: The radix tree to search.
 * @index: The index to look up.
 * Returns: The item at @index, or NULL when absent.
 */
void* radix_lookup(radix_s* tree, u32 index);

/**
 * radix_delete - Removes and returns the item at @index, or NULL when absent.
 * @tree: The radix tree to remove from.
 * @index: The index to remove.
 * Returns: The removed item, or NULL when absent.
 */
void* radix_delete(radix_s* tree, u32 index);

/**
 * radix_prune - Releases every item at or above @start, returning the count.
 * @tree: The radix tree to prune.
 * @start: The index to prune from, inclusive.
 * @release: The callback invoked on each released item.
 * Returns: The number of items released.
 */
u32 radix_prune(radix_s* tree, u32 start, radix_release_f release);
