#include <userspace/libc/string.h>
#include <userspace/vfs/hash.h>
#include <userspace/vfs/heap.h>
#include <userspace/vfs/inode.h>

#define ICACHE_BITS 6u

static hlist_head_s buckets[1u << ICACHE_BITS];
static cache_s      inode_object_cache;

static hlist_head_s* i_bucket(const super_block_s* sb, u32 ino) {
    return &buckets[hash_32((u32)(uintptr_t)sb ^ ino, ICACHE_BITS)];
}

inode_s* inode_alloc(super_block_s* sb) {
    inode_s* inode = (sb->ops && sb->ops->alloc)
        ? sb->ops->alloc(sb)
        : cache_alloc(&inode_object_cache);

    if (!inode)
        return NULL;

    memset(inode, 0, sizeof(*inode));
    inode->count = 1;
    inode->sb    = sb;
    list_init(&inode->sb_list_node);
    list_add_to_tail(&inode->sb_list_node, &sb->inodes);
    return inode;
}

void inode_insert(inode_s* inode) {
    hlist_add_head(&inode->hash_list_node, i_bucket(inode->sb, inode->ino));
}

inode_s* iget(super_block_s* sb, u32 ino) {
    hlist_head_s* bucket = i_bucket(sb, ino);
    inode_s* inode;
    hlist_for_each_entry(inode, bucket, hash_list_node) {
        if (inode->sb == sb && inode->ino == ino)
            return ihold(inode);
    }

    return NULL;
}

inode_s* ihold(inode_s* inode) {
    inode->count++;
    return inode;
}

void inode_evict(inode_s* inode) {
    super_block_s* sb = inode->sb;
    hlist_del(&inode->hash_list_node);
    list_del(&inode->sb_list_node);
    if (sb->ops && sb->ops->evict)
        sb->ops->evict(inode);

    if (sb->ops && sb->ops->destroy)
        sb->ops->destroy(inode);
    else
        cache_free(&inode_object_cache, inode);
}

void iput(inode_s* inode) {
    if (!inode || --inode->count)
        return;

    if (inode->nlink == 0)
        inode_evict(inode);
}

void inode_stat(const inode_s* inode, vfs_stat_s* stat) {
    stat->dev       = inode->sb->id;
    stat->ino       = inode->ino;
    stat->mode      = inode->mode;
    stat->nlink     = inode->nlink;
    stat->size      = inode->size;
    stat->blocks    = inode->blocks;
    stat->blocksize = inode->sb->blocksize;
}

void icache_init(void) {
    cache_init(&inode_object_cache, "inode", sizeof(inode_s));
}
