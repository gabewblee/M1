#include <userspace/libc/hash.h>
#include <userspace/libc/heap.h>
#include <userspace/vfs/fsclient.h>
#include <userspace/vfs/rnode.h>
#include <userspace/vfs/super.h>

#define RNODE_BITS 6u

static hlist_head_s buckets[1u << RNODE_BITS];
static cache_s      rnode_object_cache;

static hlist_head_s* r_bucket(const rsb_s* sb, u32 ino) {
    return &buckets[hash_32((u32)(uintptr_t)sb ^ ino, RNODE_BITS)];
}

rnode_s* rnode_get(rsb_s* sb, u32 ino, u32 mode) {
    hlist_head_s* bucket = r_bucket(sb, ino);
    rnode_s* node;
    hlist_for_each_entry(node, bucket, hash_list_node) {
        if (node->sb == sb && node->ino == ino) {
            node->count++;
            node->remote++;
            return node;
        }
    }

    node = cache_alloc(&rnode_object_cache);
    if (!node) {
        fsc_forget(sb, ino, 1);
        return NULL;
    }

    node->count  = 1;
    node->remote = 1;
    node->ino    = ino;
    node->mode   = mode;
    node->sb     = rsb_get(sb);
    hlist_add_head(&node->hash_list_node, bucket);
    return node;
}

rnode_s* rnode_hold(rnode_s* node) {
    node->count++;
    return node;
}

void rnode_put(rnode_s* node) {
    if (!node || --node->count)
        return;

    hlist_del(&node->hash_list_node);
    fsc_forget(node->sb, node->ino, node->remote);
    rsb_put(node->sb);
    cache_free(&rnode_object_cache, node);
}

i32 rnode_stat(rnode_s* node, vfs_stat_s* stat) {
    fs_entry_reply_s entry;
    i32 ret = fsc_getattr(node->sb, node->ino, &entry);
    if (ret != E_OK)
        return ret;

    stat->dev       = node->sb->dev;
    stat->ino       = entry.ino;
    stat->mode      = entry.mode;
    stat->nlink     = entry.nlink;
    stat->size      = entry.size;
    stat->blocks    = entry.blocks;
    stat->blocksize = node->sb->blocksize;
    return E_OK;
}

i32 rnode_size(rnode_s* node, i64* size) {
    fs_entry_reply_s entry;
    i32 ret = fsc_getattr(node->sb, node->ino, &entry);
    if (ret != E_OK)
        return ret;

    *size = entry.size;
    return E_OK;
}

void rnode_init(void) {
    cache_init(&rnode_object_cache, "rnode", sizeof(rnode_s));
}
