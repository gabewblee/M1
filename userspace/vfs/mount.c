#include <userspace/vfs/dcache.h>
#include <userspace/vfs/hash.h>
#include <userspace/vfs/heap.h>
#include <userspace/vfs/mount.h>
#include <userspace/vfs/namei.h>
#include <userspace/vfs/super.h>

#define MOUNT_BITS 4u

static hlist_head_s buckets[1u << MOUNT_BITS];
static cache_s      mount_object_cache;
static path_s       root;

static hlist_head_s* m_bucket(const mount_s* mount, const dentry_s* dentry) {
    return &buckets[hash_32((u32)(uintptr_t)mount ^ (u32)(uintptr_t)dentry, MOUNT_BITS)];
}

mount_s* mount_lookup(mount_s* mount, dentry_s* dentry) {
    hlist_head_s* bucket = m_bucket(mount, dentry);
    mount_s* child;
    hlist_for_each_entry(child, bucket, hash_list_node) {
        if (child->parent == mount && child->mountpoint == dentry)
            return child;
    }

    return NULL;
}

static void mount_free(mount_s* mount) {
    super_block_s* sb = mount->sb;
    dput(mount->root);
    super_kill(sb);
    cache_free(&mount_object_cache, mount);
}

mount_s* mntget(mount_s* mount) {
    if (mount)
        mount->count++;

    return mount;
}

void mntput(mount_s* mount) {
    if (mount && --mount->count == 0)
        mount_free(mount);
}

static mount_s* mount_make(fstype_s* fstype, u32 flags, const char* source) {
    mount_s* mount = cache_alloc(&mount_object_cache);
    if (!mount)
        return NULL;

    dentry_s* dentry = fstype->mount(fstype, flags, source);
    if (!dentry) {
        cache_free(&mount_object_cache, mount);
        return NULL;
    }

    mount->count      = 1;
    mount->parent     = NULL;
    mount->mountpoint = NULL;
    mount->root       = dget(dentry);
    mount->sb         = dentry->sb;
    mount->hash_list_node.next  = NULL;
    mount->hash_list_node.pprev = NULL;
    list_init(&mount->child_list_node);
    list_init(&mount->children);
    return mount;
}

i32 do_mount(const char* source, const char* target, const char* name, u32 flags) {
    fstype_s* fstype = fstype_find(name);
    if (!fstype)
        return -(i32)E_NODEV;

    path_s where;
    i32 ret = path_lookup(target, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &where);
    if (ret != E_OK)
        return ret;

    mount_s* mount = mount_make(fstype, flags, source);
    if (!mount) {
        path_put(&where);
        return -(i32)E_NOMEM;
    }

    mount->parent     = mntget(where.mount);
    mount->mountpoint = dget(where.dentry);
    hlist_add_head(&mount->hash_list_node, m_bucket(where.mount, where.dentry));
    list_add_to_tail(&mount->child_list_node, &where.mount->children);
    where.dentry->flags |= DENTRY_MOUNTED;
    path_put(&where);
    return E_OK;
}

i32 do_umount(const char* target) {
    path_s where;
    i32 ret = path_lookup(target, LOOKUP_FOLLOW, &where);
    if (ret != E_OK)
        return ret;

    mount_s* mount = where.mount;
    if (where.dentry != mount->root) {
        path_put(&where);
        return -(i32)E_INVAL;
    }

    if (!mount->parent || !list_is_empty(&mount->children) || mount->count > 2) {
        path_put(&where);
        return -(i32)E_BUSY;
    }

    hlist_del(&mount->hash_list_node);
    list_del(&mount->child_list_node);
    mount->mountpoint->flags &= ~DENTRY_MOUNTED;
    dput(mount->mountpoint);
    mntput(mount->parent);
    mount->mountpoint = NULL;
    mount->parent     = NULL;

    path_put(&where);
    mntput(mount);
    return E_OK;
}

void path_get(path_s* path) {
    mntget(path->mount);
    dget(path->dentry);
}

void path_put(path_s* path) {
    dput(path->dentry);
    mntput(path->mount);
    path->mount  = NULL;
    path->dentry = NULL;
}

void path_root(path_s* result) {
    *result = root;
    path_get(result);
}

i32 mount_init(const char* name) {
    cache_init(&mount_object_cache, "mount", sizeof(mount_s));

    fstype_s* fstype = fstype_find(name);
    if (!fstype)
        return -(i32)E_NODEV;

    mount_s* mount = mount_make(fstype, 0, "root");
    if (!mount)
        return -(i32)E_NOMEM;

    root.mount  = mntget(mount);
    root.dentry = dget(mount->root);
    return E_OK;
}
