#pragma once

#include <uapi/uapi.h>
#include <userspace/libc/list.h>

#define BUF_DIRTY 0x1u /* Buffer differs from disk */

typedef struct buf_s {
    u32          block;          /* Block number            */
    u32          flags;          /* BUF_* flags             */
    u32          count;          /* Reference count         */
    u8*          data;           /* Block contents          */
    hlist_node_s hash_list_node; /* Buffer hash chain link  */
    list_node_s  lru_list_node;  /* LRU link, valid at rest */
} buf_s;

/**
 * bcache_init - Initializes the buffer cache for @block_size byte blocks.
 * @block_size: The filesystem block size in bytes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 bcache_init(u32 block_size);

/**
 * bread - Returns a referenced buffer holding block @block, reading it from
 *         disk on a cache miss.
 * @block: The block number to read.
 * Returns: The referenced buffer, or NULL on I/O error or memory exhaustion.
 */
buf_s* bread(u32 block);

/**
 * bdirty - Marks @buffer dirty, scheduling it for writeback.
 * @buffer: The buffer to mark.
 */
void bdirty(buf_s* buffer);

/**
 * brelse - Drops a reference on @buffer, parking it on the LRU at zero.
 * @buffer: The buffer to release.
 */
void brelse(buf_s* buffer);

/**
 * bcache_sync - Writes every dirty buffer back to disk.
 * Returns: E_OK on success, or the first negative error code encountered.
 */
i32 bcache_sync(void);

/**
 * bcache_drop - Syncs and then invalidates the whole cache.
 * Returns: E_OK on success, or the first negative error code encountered.
 */
i32 bcache_drop(void);
