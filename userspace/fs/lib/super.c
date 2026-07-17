#include <userspace/fs/lib/icache.h>
#include <userspace/fs/lib/super.h>
#include <userspace/libc/heap.h>
#include <userspace/libc/string.h>

static fs_super_s* table[FS_SUPER_MAX];

i32 fs_super_mount(const fs_driver_s* driver, const char* source, u32 flags, fs_super_s** result) {
    u32 slot;
    for (slot = 0; slot < FS_SUPER_MAX; slot++)
        if (!table[slot])
            break;

    if (slot == FS_SUPER_MAX)
        return -(i32)E_BUSY;

    fs_super_s* sb = heap_alloc(sizeof(*sb));
    if (!sb)
        return -(i32)E_NOMEM;

    memset(sb, 0, sizeof(*sb));
    sb->id       = slot + 1u;
    sb->next_ino = 1;
    sb->driver   = driver;
    list_init(&sb->inodes);

    i32 ret = driver->mount(sb, source, flags);
    if (ret != E_OK) {
        heap_free(sb, sizeof(*sb));
        return ret;
    }

    table[slot] = sb;
    *result = sb;
    return E_OK;
}

fs_super_s* fs_super_find(u32 id) {
    if (id == 0 || id > FS_SUPER_MAX)
        return NULL;

    return table[id - 1u];
}

i32 fs_super_umount(fs_super_s* sb) {
    fs_inode_s* root = sb->root;
    if (root && root->count > 1)
        return -(i32)E_BUSY;

    fs_inode_s* inode;
    list_for_each_entry(inode, &sb->inodes, sb_list_node) {
        if (inode != root && inode->count > 0)
            return -(i32)E_BUSY;
    }

    sb->root = NULL;
    if (root)
        fs_iput(root, root->count);

    while (!list_is_empty(&sb->inodes)) {
        fs_inode_s* inode = list_first_entry(&sb->inodes, fs_inode_s, sb_list_node);
        fs_inode_evict(inode);
    }

    if (sb->driver->umount)
        sb->driver->umount(sb);

    table[sb->id - 1u] = NULL;
    heap_free(sb, sizeof(*sb));
    return E_OK;
}

u32 fs_super_ino(fs_super_s* sb) {
    return sb->next_ino++;
}

void fs_super_init(void) {
    for (u32 i = 0; i < FS_SUPER_MAX; i++)
        table[i] = NULL;
}
