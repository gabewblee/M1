#include <userspace/fs/lib/icache.h>
#include <userspace/libc/hash.h>

#define ICACHE_BITS 6u

static hlist_head_s buckets[1u << ICACHE_BITS];

static hlist_head_s* i_bucket(const fs_super_s* sb, u32 ino) {
    return &buckets[hash_32(sb->id ^ ino, ICACHE_BITS)];
}

fs_inode_s* fs_inode_new(fs_super_s* sb) {
    fs_inode_s* inode = sb->driver->ialloc(sb);
    if (!inode)
        return NULL;

    inode->ino    = 0;
    inode->mode   = 0;
    inode->nlink  = 0;
    inode->count  = 1;
    inode->size   = 0;
    inode->blocks = 0;
    inode->sb     = sb;
    inode->hash_list_node.next  = NULL;
    inode->hash_list_node.pprev = NULL;
    list_init(&inode->sb_list_node);
    list_add_to_tail(&inode->sb_list_node, &sb->inodes);
    return inode;
}

void fs_inode_hash(fs_inode_s* inode) {
    hlist_add_head(&inode->hash_list_node, i_bucket(inode->sb, inode->ino));
}

fs_inode_s* fs_ilookup(fs_super_s* sb, u32 ino) {
    hlist_head_s* bucket = i_bucket(sb, ino);
    fs_inode_s* inode;
    hlist_for_each_entry(inode, bucket, hash_list_node) {
        if (inode->sb == sb && inode->ino == ino)
            return inode;
    }

    return NULL;
}

fs_inode_s* fs_iget(fs_super_s* sb, u32 ino) {
    fs_inode_s* inode = fs_ilookup(sb, ino);
    return inode ? fs_ihold(inode) : NULL;
}

fs_inode_s* fs_ihold(fs_inode_s* inode) {
    inode->count++;
    return inode;
}

void fs_inode_evict(fs_inode_s* inode) {
    const fs_driver_s* driver = inode->sb->driver;
    hlist_del(&inode->hash_list_node);
    list_del(&inode->sb_list_node);
    if (driver->ievict)
        driver->ievict(inode);

    driver->ifree(inode);
}

void fs_iput(fs_inode_s* inode, u32 count) {
    if (!inode)
        return;

    inode->count = (count < inode->count) ? inode->count - count : 0;
    if (inode->count)
        return;

    if (inode->nlink == 0 || !(inode->sb->flags & FS_SUPER_KEEP_CACHE))
        fs_inode_evict(inode);
}

void fs_icache_init(void) {
    for (u32 i = 0; i < (1u << ICACHE_BITS); i++)
        buckets[i].first = NULL;
}
