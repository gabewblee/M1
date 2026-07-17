#include <userspace/libc/hash.h>
#include <userspace/libc/heap.h>
#include <userspace/vfs/dcache.h>
#include <userspace/vfs/fsclient.h>
#include <userspace/vfs/mount.h>
#include <userspace/vfs/namei.h>
#include <userspace/vfs/rnode.h>
#include <userspace/vfs/super.h>

#define MOUNT_BITS 4u

static hlist_head_s buckets[1u << MOUNT_BITS];
static list_node_s  mounts;
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
    rsb_s* sb = mount->sb;
    list_del(&mount->mount_list_node);
    dcache_prune_sb(sb);
    dput(mount->root);
    rsb_put(sb);
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

static dentry_s* root_dentry_make(rsb_s* sb) {
    fs_entry_reply_s entry;
    i32 ret = fsc_getattr(sb, sb->root_ino, &entry);
    if (ret != E_OK)
        return NULL;

    rnode_s* node = rnode_get(sb, sb->root_ino, entry.mode);
    if (!node)
        return NULL;

    qstr_s name = { "/", 1, hash_full_name("/", 1) };
    dentry_s* dentry = d_alloc(NULL, &name);
    if (!dentry) {
        rnode_put(node);
        return NULL;
    }

    dentry->sb = sb;
    d_instantiate(dentry, node);
    return dentry;
}

static mount_s* mount_make(const fstype_s* fstype, u32 flags, const char* source) {
    mount_s* mount = cache_alloc(&mount_object_cache);
    if (!mount)
        return NULL;

    rsb_s* sb;
    if (rsb_mount(fstype, source, flags, &sb) != E_OK) {
        cache_free(&mount_object_cache, mount);
        return NULL;
    }

    dentry_s* dentry = root_dentry_make(sb);
    if (!dentry) {
        rsb_put(sb);
        cache_free(&mount_object_cache, mount);
        return NULL;
    }

    mount->count      = 1;
    mount->parent     = NULL;
    mount->mountpoint = NULL;
    mount->root       = dentry;
    mount->sb         = sb;
    mount->hash_list_node.next  = NULL;
    mount->hash_list_node.pprev = NULL;
    list_init(&mount->child_list_node);
    list_init(&mount->children);
    list_add_to_tail(&mount->mount_list_node, &mounts);
    return mount;
}

i32 do_mount(const char* source, const char* target, const char* name, u32 flags) {
    const fstype_s* fstype = fstype_find(name);
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

i32 do_statfs(const char* path, vfs_statfs_s* statfs) {
    path_s where;
    i32 ret = path_lookup(path, LOOKUP_FOLLOW, &where);
    if (ret != E_OK)
        return ret;

    ret = fsc_statfs(where.mount->sb, statfs);
    path_put(&where);
    return ret;
}

i32 do_sync(void) {
    i32 ret = E_OK;
    mount_s* mount;
    list_for_each_entry(mount, &mounts, mount_list_node) {
        i32 err = fsc_sync(mount->sb);
        if (err != E_OK && ret == E_OK)
            ret = err;
    }

    return ret;
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
    list_init(&mounts);

    const fstype_s* fstype = fstype_find(name);
    if (!fstype)
        return -(i32)E_NODEV;

    mount_s* mount = mount_make(fstype, 0, "root");
    if (!mount)
        return -(i32)E_NOMEM;

    root.mount  = mntget(mount);
    root.dentry = dget(mount->root);
    return E_OK;
}
