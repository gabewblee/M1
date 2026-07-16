#include <userspace/libc/string.h>
#include <userspace/vfs/dcache.h>
#include <userspace/vfs/libfs.h>

dentry_s* simple_lookup(inode_s* dir, dentry_s* dentry) {
    d_add(dentry, NULL);
    return NULL;
}

int simple_empty(dentry_s* dentry) {
    dentry_s* child;
    list_for_each_entry(child, &dentry->children, child_list_node) {
        if (!d_is_negative(child))
            return 0;
    }

    return 1;
}

i32 simple_unlink(inode_s* dir, dentry_s* dentry) {
    dentry->inode->nlink--;
    dput(dentry);
    return E_OK;
}

i32 simple_rmdir(inode_s* dir, dentry_s* dentry) {
    if (!simple_empty(dentry))
        return -(i32)E_NOTEMPTY;

    dentry->inode->nlink--;
    simple_unlink(dir, dentry);
    dir->nlink--;
    return E_OK;
}

i32 simple_rename(inode_s* source, dentry_s* from, inode_s* target, dentry_s* to) {
    int directory = d_is_dir(from);
    if (!simple_empty(to))
        return -(i32)E_NOTEMPTY;

    if (!d_is_negative(to)) {
        simple_unlink(target, to);
        if (directory) {
            to->inode->nlink--;
            source->nlink--;
        }
    } else if (directory) {
        source->nlink--;
        target->nlink++;
    }

    return E_OK;
}

i32 generic_seek(file_s* file, i64 offset, u32 whence, i64* result) {
    i64 base;
    switch (whence) {
        case VFS_SEEK_SET:
            base = 0;
            break;

        case VFS_SEEK_CUR:
            base = file->pos;
            break;

        case VFS_SEEK_END:
            base = file->path.dentry->inode->size;
            break;

        default:
            return -(i32)E_INVAL;
    }

    i64 position = base + offset;
    if (position < 0)
        return -(i32)E_INVAL;

    file->pos = position;
    *result   = position;
    return E_OK;
}

static void dirent_fill(vfs_dirent_s* entry, const inode_s* inode, const char* name, u32 len) {
    entry->ino  = inode->ino;
    entry->type = VFS_DT(inode->mode & VFS_IFMT);
    if (len >= VFS_NAME_MAX)
        len = VFS_NAME_MAX - 1u;

    memcpy(entry->name, name, len);
    entry->name[len] = '\0';
}

i32 dcache_readdir(file_s* file, vfs_dirent_s* entry) {
    dentry_s* dentry = file->path.dentry;
    if (file->pos == 0) {
        dirent_fill(entry, dentry->inode, ".", 1);
        file->pos = 1;
        return 1;
    }

    if (file->pos == 1) {
        dirent_fill(entry, dentry->parent->inode, "..", 2);
        file->pos = 2;
        return 1;
    }

    list_node_s* head = &dentry->children;
    list_node_s* node = NULL;
    dentry_s* cursor = file->private;
    if (cursor && cursor->parent == dentry && list_is_attached(&cursor->child_list_node))
        node = cursor->child_list_node.next;

    if (!node) {
        i64 skip = file->pos - 2;
        node = head->next;
        while (node != head && skip > 0) {
            if (!d_is_negative(list_entry(node, dentry_s, child_list_node)))
                skip--;

            node = node->next;
        }
    }

    while (node != head && d_is_negative(list_entry(node, dentry_s, child_list_node)))
        node = node->next;

    if (node == head)
        return 0;

    dentry_s* child = list_entry(node, dentry_s, child_list_node);
    dirent_fill(entry, child->inode, child->name.name, child->name.len);
    if (cursor)
        dput(cursor);

    file->private = dget(child);
    file->pos++;
    return 1;
}

i32 dcache_dir_seek(file_s* file, i64 offset, u32 whence, i64* result) {
    i32 ret = generic_seek(file, offset, whence, result);
    if (ret == E_OK && file->private) {
        dput(file->private);
        file->private = NULL;
    }

    return ret;
}

i32 dcache_dir_release(inode_s* inode, file_s* file) {
    if (file->private) {
        dput(file->private);
        file->private = NULL;
    }

    return E_OK;
}

const file_ops_s simple_dir_ops = {
    .llseek  = dcache_dir_seek,
    .readdir = dcache_readdir,
    .release = dcache_dir_release,
};
