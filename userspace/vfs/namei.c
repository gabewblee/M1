#include <userspace/libc/string.h>
#include <userspace/vfs/dcache.h>
#include <userspace/vfs/file.h>
#include <userspace/vfs/hash.h>
#include <userspace/vfs/inode.h>
#include <userspace/vfs/mount.h>
#include <userspace/vfs/namei.h>

enum { LAST_NORMAL, LAST_ROOT, LAST_DOT, LAST_DOTDOT };

typedef struct nameidata_s {
    path_s path;  /* Current walk position */
    qstr_s last;  /* Final component       */
    u32    kind;  /* LAST_* class of last  */
    u32    flags; /* LOOKUP_* flags        */
    u32    depth; /* Symlink nesting depth */
} nameidata_s;

static void follow_mounts(path_s* path) {
    while (d_is_mounted(path->dentry)) {
        mount_s* child = mount_lookup(path->mount, path->dentry);
        if (!child)
            break;

        mntget(child);
        dget(child->root);
        dput(path->dentry);
        mntput(path->mount);
        path->mount  = child;
        path->dentry = child->root;
    }
}

static void follow_dotdot(nameidata_s* walk) {
    for (;;) {
        dentry_s* dentry = walk->path.dentry;
        if (dentry != walk->path.mount->root) {
            walk->path.dentry = dget(dentry->parent);
            dput(dentry);
            break;
        }

        mount_s* mount = walk->path.mount;
        if (!mount->parent)
            break;

        dentry_s* point = dget(mount->mountpoint);
        mount_s* parent = mntget(mount->parent);
        dput(dentry);
        mntput(mount);
        walk->path.mount  = parent;
        walk->path.dentry = point;
    }

    follow_mounts(&walk->path);
}

static i32 lookup_dentry(nameidata_s* walk, const qstr_s* name, dentry_s** result) {
    dentry_s* parent = walk->path.dentry;
    dentry_s* dentry = d_lookup(parent, name);
    if (!dentry) {
        inode_s* dir = parent->inode;
        if (!dir->ops || !dir->ops->lookup)
            return -(i32)E_NOSYS;

        dentry = d_alloc(parent, name);
        if (!dentry)
            return -(i32)E_NOMEM;

        dentry_s* found = dir->ops->lookup(dir, dentry);
        if (found) {
            dput(dentry);
            dentry = found;
        }
    }

    *result = dentry;
    return E_OK;
}

static i32 walk_path(nameidata_s* walk, const char* path) {
    if (*path == '/') {
        path_put(&walk->path);
        path_root(&walk->path);
        while (*path == '/')
            path++;
    }

    if (*path == '\0') {
        walk->kind = LAST_ROOT;
        return E_OK;
    }

    for (;;) {
        qstr_s name;
        u32 hash = 0, len = 0;
        name.name = path;
        while (path[len] != '\0' && path[len] != '/') {
            hash = hash_partial_name((u8)path[len], hash);
            len++;
        }

        if (len >= VFS_NAME_MAX)
            return -(i32)E_NAMELONG;

        name.len  = len;
        name.hash = hash;
        path += len;

        u32 kind = LAST_NORMAL;
        if (name.name[0] == '.') {
            if (len == 1)
                kind = LAST_DOT;
            else if (len == 2 && name.name[1] == '.')
                kind = LAST_DOTDOT;
        }

        u32 trailing = 0;
        while (*path == '/') {
            path++;
            trailing = 1;
        }

        int final = (*path == '\0');
        if (final && trailing)
            walk->flags |= LOOKUP_DIRECTORY;

        if (!d_is_dir(walk->path.dentry))
            return -(i32)E_NOTDIR;

        if (final && (walk->flags & LOOKUP_PARENT)) {
            walk->last = name;
            walk->kind = kind;
            return E_OK;
        }

        if (kind == LAST_DOT) {
            if (final)
                return E_OK;

            continue;
        }

        if (kind == LAST_DOTDOT) {
            follow_dotdot(walk);
            if (final)
                return E_OK;

            continue;
        }

        dentry_s* dentry;
        i32 ret = lookup_dentry(walk, &name, &dentry);
        if (ret != E_OK)
            return ret;

        if (d_is_negative(dentry)) {
            dput(dentry);
            return -(i32)E_NOENT;
        }

        inode_s* inode = dentry->inode;
        if (VFS_ISLNK(inode->mode) && (!final || trailing || (walk->flags & LOOKUP_FOLLOW))) {
            if (walk->depth >= VFS_SYMLINK_MAX) {
                dput(dentry);
                return -(i32)E_LOOP;
            }

            const char* content = (inode->ops && inode->ops->getlink)
                ? inode->ops->getlink(inode)
                : NULL;

            if (!content || *content == '\0') {
                dput(dentry);
                return -(i32)E_NOENT;
            }

            u32 saved = walk->flags;
            if (!final)
                walk->flags = LOOKUP_FOLLOW;

            walk->depth++;
            ret = walk_path(walk, content);
            walk->depth--;
            walk->flags = saved;
            dput(dentry);
            if (ret != E_OK)
                return ret;

            if (final)
                return E_OK;

            continue;
        }

        dput(walk->path.dentry);
        walk->path.dentry = dentry;
        follow_mounts(&walk->path);

        if (final) {
            if ((walk->flags & LOOKUP_DIRECTORY) && !d_is_dir(walk->path.dentry))
                return -(i32)E_NOTDIR;

            return E_OK;
        }
    }
}

static i32 walk_begin(nameidata_s* walk, const char* path, u32 flags) {
    if (!path || *path == '\0')
        return -(i32)E_NOENT;

    walk->flags     = flags;
    walk->depth     = 0;
    walk->kind      = LAST_ROOT;
    walk->last.name = NULL;
    walk->last.len  = 0;
    walk->last.hash = 0;
    path_root(&walk->path);

    i32 ret = walk_path(walk, path);
    if (ret != E_OK)
        path_put(&walk->path);

    return ret;
}

i32 path_lookup(const char* path, u32 flags, path_s* result) {
    nameidata_s walk;
    i32 ret = walk_begin(&walk, path, flags);
    if (ret != E_OK)
        return ret;

    *result = walk.path;
    return E_OK;
}

static i32 path_parent(const char* path, nameidata_s* walk) {
    return walk_begin(walk, path, LOOKUP_PARENT);
}

i32 do_mkdir(const char* path, u32 mode) {
    nameidata_s walk;
    i32 ret = path_parent(path, &walk);
    if (ret != E_OK)
        return ret;

    if (walk.kind != LAST_NORMAL) {
        path_put(&walk.path);
        return -(i32)E_EXIST;
    }

    dentry_s* dentry;
    ret = lookup_dentry(&walk, &walk.last, &dentry);
    if (ret == E_OK) {
        inode_s* dir = walk.path.dentry->inode;
        if (!d_is_negative(dentry))
            ret = -(i32)E_EXIST;
        else if (!dir->ops || !dir->ops->mkdir)
            ret = -(i32)E_NOSYS;
        else
            ret = dir->ops->mkdir(dir, dentry, (mode & ~VFS_IFMT) | VFS_IFDIR);

        dput(dentry);
    }

    path_put(&walk.path);
    return ret;
}

i32 do_unlink(const char* path) {
    nameidata_s walk;
    i32 ret = path_parent(path, &walk);
    if (ret != E_OK)
        return ret;

    if (walk.kind != LAST_NORMAL) {
        path_put(&walk.path);
        return -(i32)E_INVAL;
    }

    dentry_s* dentry;
    ret = lookup_dentry(&walk, &walk.last, &dentry);
    if (ret == E_OK) {
        inode_s* dir = walk.path.dentry->inode;
        if (d_is_negative(dentry))
            ret = -(i32)E_NOENT;
        else if (d_is_dir(dentry))
            ret = -(i32)E_ISDIR;
        else if (!dir->ops || !dir->ops->unlink)
            ret = -(i32)E_NOSYS;
        else
            ret = dir->ops->unlink(dir, dentry);

        if (ret == E_OK)
            d_delete(dentry);

        dput(dentry);
    }

    path_put(&walk.path);
    return ret;
}

i32 do_rmdir(const char* path) {
    nameidata_s walk;
    i32 ret = path_parent(path, &walk);
    if (ret != E_OK)
        return ret;

    if (walk.kind != LAST_NORMAL) {
        path_put(&walk.path);
        return -(i32)E_INVAL;
    }

    dentry_s* dentry;
    ret = lookup_dentry(&walk, &walk.last, &dentry);
    if (ret == E_OK) {
        inode_s* dir = walk.path.dentry->inode;
        if (d_is_negative(dentry))
            ret = -(i32)E_NOENT;
        else if (!d_is_dir(dentry))
            ret = -(i32)E_NOTDIR;
        else if (d_is_mounted(dentry))
            ret = -(i32)E_BUSY;
        else if (!dir->ops || !dir->ops->rmdir)
            ret = -(i32)E_NOSYS;
        else
            ret = dir->ops->rmdir(dir, dentry);

        if (ret == E_OK)
            d_delete(dentry);

        dput(dentry);
    }

    path_put(&walk.path);
    return ret;
}

i32 do_link(const char* source, const char* target) {
    path_s old;
    i32 ret = path_lookup(source, 0, &old);
    if (ret != E_OK)
        return ret;

    nameidata_s walk;
    ret = path_parent(target, &walk);
    if (ret != E_OK) {
        path_put(&old);
        return ret;
    }

    if (walk.kind != LAST_NORMAL)
        ret = -(i32)E_EXIST;
    else if (old.mount != walk.path.mount)
        ret = -(i32)E_XDEV;
    else if (d_is_dir(old.dentry))
        ret = -(i32)E_PERM;
    else {
        dentry_s* dentry;
        ret = lookup_dentry(&walk, &walk.last, &dentry);
        if (ret == E_OK) {
            inode_s* dir = walk.path.dentry->inode;
            if (!d_is_negative(dentry))
                ret = -(i32)E_EXIST;
            else if (!dir->ops || !dir->ops->link)
                ret = -(i32)E_NOSYS;
            else
                ret = dir->ops->link(old.dentry, dir, dentry);

            dput(dentry);
        }
    }

    path_put(&walk.path);
    path_put(&old);
    return ret;
}

i32 do_symlink(const char* content, const char* target) {
    if (!content || *content == '\0')
        return -(i32)E_NOENT;

    nameidata_s walk;
    i32 ret = path_parent(target, &walk);
    if (ret != E_OK)
        return ret;

    if (walk.kind != LAST_NORMAL) {
        path_put(&walk.path);
        return -(i32)E_EXIST;
    }

    dentry_s* dentry;
    ret = lookup_dentry(&walk, &walk.last, &dentry);
    if (ret == E_OK) {
        inode_s* dir = walk.path.dentry->inode;
        if (!d_is_negative(dentry))
            ret = -(i32)E_EXIST;
        else if (!dir->ops || !dir->ops->symlink)
            ret = -(i32)E_NOSYS;
        else
            ret = dir->ops->symlink(dir, dentry, content);

        dput(dentry);
    }

    path_put(&walk.path);
    return ret;
}

i32 do_rename(const char* source, const char* target) {
    nameidata_s from, to;
    i32 ret = path_parent(source, &from);
    if (ret != E_OK)
        return ret;

    ret = path_parent(target, &to);
    if (ret != E_OK) {
        path_put(&from.path);
        return ret;
    }

    dentry_s* subject = NULL;
    dentry_s* victim  = NULL;
    if (from.kind != LAST_NORMAL || to.kind != LAST_NORMAL)
        ret = -(i32)E_INVAL;
    else if (from.path.mount != to.path.mount)
        ret = -(i32)E_XDEV;
    else {
        ret = lookup_dentry(&from, &from.last, &subject);
        if (ret == E_OK)
            ret = lookup_dentry(&to, &to.last, &victim);
    }

    if (ret == E_OK) {
        inode_s* origin      = from.path.dentry->inode;
        inode_s* destination = to.path.dentry->inode;
        if (d_is_negative(subject))
            ret = -(i32)E_NOENT;
        else if (subject == victim)
            ret = E_OK;
        else if (d_is_mounted(subject) || d_is_mounted(victim))
            ret = -(i32)E_BUSY;
        else if (d_is_ancestor(subject, to.path.dentry))
            ret = -(i32)E_INVAL;
        else if (!d_is_negative(victim) && d_is_dir(subject) && !d_is_dir(victim))
            ret = -(i32)E_NOTDIR;
        else if (!d_is_negative(victim) && !d_is_dir(subject) && d_is_dir(victim))
            ret = -(i32)E_ISDIR;
        else if (!origin->ops || !origin->ops->rename)
            ret = -(i32)E_NOSYS;
        else {
            ret = origin->ops->rename(origin, subject, destination, victim);
            if (ret == E_OK)
                d_move(subject, victim);
        }
    }

    dput(subject);
    dput(victim);
    path_put(&to.path);
    path_put(&from.path);
    return ret;
}

i32 do_truncate(const char* path, i64 length) {
    if (length < 0)
        return -(i32)E_INVAL;

    path_s where;
    i32 ret = path_lookup(path, LOOKUP_FOLLOW, &where);
    if (ret != E_OK)
        return ret;

    inode_s* inode = where.dentry->inode;
    if (VFS_ISDIR(inode->mode))
        ret = -(i32)E_ISDIR;
    else if (!VFS_ISREG(inode->mode))
        ret = -(i32)E_INVAL;
    else if (!inode->ops || !inode->ops->truncate)
        ret = -(i32)E_NOSYS;
    else
        ret = inode->ops->truncate(inode, length);

    path_put(&where);
    return ret;
}

i32 do_readlink(const char* path, char* buffer, u32 size) {
    path_s where;
    i32 ret = path_lookup(path, 0, &where);
    if (ret != E_OK)
        return ret;

    inode_s* inode = where.dentry->inode;
    if (!VFS_ISLNK(inode->mode)) {
        ret = -(i32)E_INVAL;
    } else {
        const char* content = (inode->ops && inode->ops->getlink)
            ? inode->ops->getlink(inode)
            : NULL;

        if (!content) {
            ret = -(i32)E_NOSYS;
        } else {
            u32 length = (u32)strlen(content);
            if (length > size)
                length = size;

            memcpy(buffer, content, length);
            ret = (i32)length;
        }
    }

    path_put(&where);
    return ret;
}

i32 do_stat(const char* path, u32 flags, vfs_stat_s* stat) {
    path_s where;
    i32 ret = path_lookup(path, (flags & VFS_O_NOFOLLOW) ? 0 : LOOKUP_FOLLOW, &where);
    if (ret != E_OK)
        return ret;

    inode_stat(where.dentry->inode, stat);
    path_put(&where);
    return E_OK;
}

i32 do_open(const char* path, u32 flags, u32 mode, file_s** result) {
    path_s where = { NULL, NULL };
    i32 ret = E_OK;

    if (flags & VFS_O_CREAT) {
        nameidata_s walk;
        ret = path_parent(path, &walk);
        if (ret != E_OK)
            return ret;

        if (walk.kind != LAST_NORMAL) {
            path_put(&walk.path);
            return -(i32)E_ISDIR;
        }

        dentry_s* dentry;
        ret = lookup_dentry(&walk, &walk.last, &dentry);
        if (ret != E_OK) {
            path_put(&walk.path);
            return ret;
        }

        if (d_is_negative(dentry)) {
            inode_s* dir = walk.path.dentry->inode;
            ret = (dir->ops && dir->ops->create)
                ? dir->ops->create(dir, dentry, (mode & ~VFS_IFMT) | VFS_IFREG)
                : -(i32)E_NOSYS;
        } else if (flags & VFS_O_EXCL) {
            ret = -(i32)E_EXIST;
        }

        if (ret != E_OK) {
            dput(dentry);
            path_put(&walk.path);
            return ret;
        }

        where.mount  = mntget(walk.path.mount);
        where.dentry = dentry;
        path_put(&walk.path);
        follow_mounts(&where);

        if (VFS_ISLNK(where.dentry->inode->mode) && !(flags & VFS_O_NOFOLLOW)) {
            path_put(&where);
            ret = path_lookup(path, LOOKUP_FOLLOW, &where);
            if (ret != E_OK)
                return ret;
        }
    } else {
        u32 lookup = (flags & VFS_O_NOFOLLOW) ? 0 : LOOKUP_FOLLOW;
        if (flags & VFS_O_DIRECTORY)
            lookup |= LOOKUP_DIRECTORY;

        ret = path_lookup(path, lookup, &where);
        if (ret != E_OK)
            return ret;
    }

    inode_s* inode = where.dentry->inode;
    u32 access = flags & VFS_O_ACCMODE;
    if (VFS_ISLNK(inode->mode))
        ret = -(i32)E_LOOP;
    else if ((flags & VFS_O_DIRECTORY) && !VFS_ISDIR(inode->mode))
        ret = -(i32)E_NOTDIR;
    else if (VFS_ISDIR(inode->mode) && access != VFS_O_RDONLY)
        ret = -(i32)E_ISDIR;
    else if ((flags & VFS_O_TRUNC) && VFS_ISREG(inode->mode) && access != VFS_O_RDONLY)
        ret = (inode->ops && inode->ops->truncate)
            ? inode->ops->truncate(inode, 0)
            : -(i32)E_NOSYS;

    if (ret != E_OK) {
        path_put(&where);
        return ret;
    }

    return file_open(&where, flags, result);
}
