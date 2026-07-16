#include <userspace/libc/string.h>
#include <userspace/vfs/heap.h>
#include <userspace/vfs/inode.h>
#include <userspace/vfs/super.h>

static fstype_s* registry;
static cache_s   super_object_cache;
static u32       next_device_number = 1;

i32 fstype_register(fstype_s* fstype) {
    if (fstype_find(fstype->name))
        return -(i32)E_EXIST;

    fstype->next = registry;
    registry = fstype;
    return E_OK;
}

fstype_s* fstype_find(const char* name) {
    for (fstype_s* fstype = registry; fstype; fstype = fstype->next)
        if (strcmp(fstype->name, name) == 0)
            return fstype;

    return NULL;
}

super_block_s* super_alloc(const fstype_s* fstype) {
    super_block_s* sb = cache_alloc(&super_object_cache);
    if (!sb)
        return NULL;

    memset(sb, 0, sizeof(*sb));
    sb->id                = next_device_number++;
    sb->blocksize         = PG_SZ;
    sb->next_inode_number = 1;
    sb->fstype            = fstype;
    list_init(&sb->inodes);
    return sb;
}

void super_kill(super_block_s* sb) {
    if (sb->fstype->kill)
        sb->fstype->kill(sb);

    while (!list_is_empty(&sb->inodes)) {
        inode_s* inode = list_first_entry(&sb->inodes, inode_s, sb_list_node);
        inode_evict(inode);
    }

    cache_free(&super_object_cache, sb);
}

u32 super_ino(super_block_s* sb) {
    return sb->next_inode_number++;
}

void super_init(void) {
    cache_init(&super_object_cache, "super", sizeof(super_block_s));
}
