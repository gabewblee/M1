#include <uapi/ata.h>
#include <userspace/fs/ext2/bcache.h>
#include <userspace/fs/ext2/blk.h>
#include <userspace/libc/hash.h>
#include <userspace/libc/heap.h>

#define BCACHE_BUFS 32u
#define BCACHE_BITS 5u

static buf_s        bufs[BCACHE_BUFS];
static hlist_head_s buckets[1u << BCACHE_BITS];
static list_node_s  lru;
static u32          bsize;
static u32          spb;

static hlist_head_s* b_bucket(u32 block) {
    return &buckets[hash_32(block, BCACHE_BITS)];
}

i32 bcache_init(u32 block_size) {
    u32 first = !bsize;
    bsize = block_size;
    spb   = block_size / ATA_SECTOR_SZ;
    if (!first)
        return E_OK;

    list_init(&lru);
    for (u32 i = 0; i < BCACHE_BUFS; i++) {
        bufs[i].data = pg_alloc();
        if (!bufs[i].data)
            return -(i32)E_NOMEM;

        bufs[i].block                = 0;
        bufs[i].flags                = 0;
        bufs[i].count                = 0;
        bufs[i].hash_list_node.next  = NULL;
        bufs[i].hash_list_node.pprev = NULL;
        list_add_to_tail(&bufs[i].lru_list_node, &lru);
    }

    return E_OK;
}

static i32 bwrite(buf_s* buffer) {
    i32 ret = blk_write((u64)buffer->block * spb, spb, buffer->data);
    if (ret == E_OK)
        buffer->flags &= ~BUF_DIRTY;

    return ret;
}

static buf_s* buf_lookup(u32 block) {
    hlist_head_s* bucket = b_bucket(block);
    buf_s* buffer;
    hlist_for_each_entry(buffer, bucket, hash_list_node) {
        if (buffer->block == block)
            return buffer;
    }

    return NULL;
}

static buf_s* buf_victim(void) {
    buf_s* buffer;
    list_for_each_entry(buffer, &lru, lru_list_node) {
        if (!(buffer->flags & BUF_DIRTY) || bwrite(buffer) == E_OK)
            return buffer;
    }

    return NULL;
}

buf_s* bread(u32 block) {
    buf_s* buffer = buf_lookup(block);
    if (buffer) {
        if (buffer->count++ == 0)
            list_del(&buffer->lru_list_node);

        return buffer;
    }

    buffer = buf_victim();
    if (!buffer)
        return NULL;

    list_del(&buffer->lru_list_node);
    hlist_del(&buffer->hash_list_node);
    if (blk_read((u64)block * spb, spb, buffer->data) != E_OK) {
        list_add_to_tail(&buffer->lru_list_node, &lru);
        return NULL;
    }

    buffer->block = block;
    buffer->count = 1;
    hlist_add_head(&buffer->hash_list_node, b_bucket(block));
    return buffer;
}

void bdirty(buf_s* buffer) {
    buffer->flags |= BUF_DIRTY;
}

void brelse(buf_s* buffer) {
    if (buffer && --buffer->count == 0)
        list_add_to_tail(&buffer->lru_list_node, &lru);
}

i32 bcache_sync(void) {
    i32 ret = E_OK;
    for (u32 i = 0; i < BCACHE_BUFS; i++) {
        if (!(bufs[i].flags & BUF_DIRTY))
            continue;

        i32 err = bwrite(&bufs[i]);
        if (err != E_OK && ret == E_OK)
            ret = err;
    }

    return ret;
}

i32 bcache_drop(void) {
    i32 ret = bcache_sync();
    for (u32 i = 0; i < BCACHE_BUFS; i++) {
        if (bufs[i].count)
            continue;

        hlist_del(&bufs[i].hash_list_node);
        bufs[i].hash_list_node.next  = NULL;
        bufs[i].hash_list_node.pprev = NULL;
        bufs[i].block                = 0;
        bufs[i].flags                = 0;
    }

    return ret;
}
