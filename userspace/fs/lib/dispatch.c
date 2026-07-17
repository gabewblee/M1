#include <userspace/fs/lib/dispatch.h>
#include <userspace/fs/lib/icache.h>
#include <userspace/fs/lib/super.h>
#include <userspace/libc/heap.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/server/server.h>

FS_SERVER_OPS(SERVER_OP_DECL)

static const server_handler_f fs_handlers[SERVER_OP_MAX] = {
    FS_SERVER_OPS(SERVER_OP_ENTRY)
};

static const fs_driver_s* fs_driver;

static i32 check_badge(u32 badge) {
    return badge == SERVER_BADGE(SERVER_ID_vfs) ? E_OK : -(i32)E_PERM;
}

static void entry_fill(fs_entry_reply_s* reply, const fs_inode_s* inode) {
    reply->ret    = E_OK;
    reply->ino    = inode->ino;
    reply->mode   = inode->mode;
    reply->nlink  = inode->nlink;
    reply->size   = inode->size;
    reply->blocks = inode->blocks;
}

static i32 rep_entry(ipc_msg_s* msg, i32 ret, const fs_inode_s* inode) {
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_entry_reply_s reply;
    entry_fill(&reply, inode);
    memcpy(msg->payload, &reply, sizeof(reply));
    return (i32)sizeof(reply);
}

static fs_inode_s* dir_of(u32 sb_id, u32 ino) {
    fs_super_s* sb = fs_super_find(sb_id);
    if (!sb)
        return NULL;

    fs_inode_s* dir = fs_ilookup(sb, ino);
    if (!dir || !VFS_ISDIR(dir->mode))
        return NULL;

    return dir;
}

static i32 handle_mount(ipc_msg_s* msg, u32 badge) {
    fs_mount_req_s* req            = (fs_mount_req_s*)msg->payload;
    req->source[FS_SOURCE_MAX - 1] = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb;
    ret = fs_super_mount(fs_driver, req->source, req->flags, &sb);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_mount_reply_s reply = {
        .ret       = E_OK,
        .sb        = sb->id,
        .root      = fs_ihold(sb->root)->ino,
        .blocksize = sb->blocksize,
        .magic     = sb->magic,
    };

    memcpy(msg->payload, &reply, sizeof(reply));
    return (i32)sizeof(reply);
}

static i32 handle_umount(ipc_msg_s* msg, u32 badge) {
    fs_sb_req_s* req = (fs_sb_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    if (!sb)
        return rep_stat_only(msg, -(i32)E_INVAL);

    return rep_stat_only(msg, fs_super_umount(sb));
}

static i32 handle_sync(ipc_msg_s* msg, u32 badge) {
    fs_sb_req_s* req = (fs_sb_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    if (!sb)
        return rep_stat_only(msg, -(i32)E_INVAL);

    return rep_stat_only(msg, fs_driver->sync ? fs_driver->sync(sb) : E_OK);
}

static i32 handle_statfs(ipc_msg_s* msg, u32 badge) {
    fs_sb_req_s* req = (fs_sb_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    if (!sb)
        return rep_stat_only(msg, -(i32)E_INVAL);

    fs_statfs_reply_s reply = { .ret = E_OK, .blocksize = sb->blocksize };
    vfs_statfs_s statfs = {0};
    if (fs_driver->statfs) {
        fs_driver->statfs(sb, &statfs);
        reply.blocks    = statfs.blocks;
        reply.bfree     = statfs.bfree;
        reply.files     = statfs.files;
        reply.ffree     = statfs.ffree;
        reply.blocksize = statfs.blocksize;
    }

    memcpy(msg->payload, &reply, sizeof(reply));
    return (i32)sizeof(reply);
}

static i32 handle_lookup(ipc_msg_s* msg, u32 badge) {
    fs_name_req_s* req          = (fs_name_req_s*)msg->payload;
    req->name[VFS_NAME_MAX - 1] = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_inode_s* dir = dir_of(req->sb, req->dir);
    if (!dir)
        return rep_stat_only(msg, -(i32)E_NOTDIR);

    fs_inode_s* inode = NULL;
    ret = fs_driver->lookup(dir, req->name, &inode);
    return rep_entry(msg, ret, inode);
}

static i32 handle_forget(ipc_msg_s* msg, u32 badge) {
    fs_node_req_s* req = (fs_node_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    if (!sb)
        return rep_stat_only(msg, -(i32)E_INVAL);

    fs_iput(fs_ilookup(sb, req->ino), req->count);
    return rep_stat_only(msg, E_OK);
}

static i32 handle_getattr(ipc_msg_s* msg, u32 badge) {
    fs_node_req_s* req = (fs_node_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    fs_inode_s* inode = sb ? fs_ilookup(sb, req->ino) : NULL;
    if (!inode)
        return rep_stat_only(msg, -(i32)E_NOENT);

    return rep_entry(msg, E_OK, inode);
}

static i32 handle_create(ipc_msg_s* msg, u32 badge) {
    fs_name_req_s* req          = (fs_name_req_s*)msg->payload;
    req->name[VFS_NAME_MAX - 1] = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_inode_s* dir = dir_of(req->sb, req->dir);
    if (!dir)
        return rep_stat_only(msg, -(i32)E_NOTDIR);

    fs_inode_s* inode = NULL;
    ret = fs_driver->create(dir, req->name, req->mode, &inode);
    return rep_entry(msg, ret, inode);
}

static i32 handle_mkdir(ipc_msg_s* msg, u32 badge) {
    fs_name_req_s* req          = (fs_name_req_s*)msg->payload;
    req->name[VFS_NAME_MAX - 1] = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_inode_s* dir = dir_of(req->sb, req->dir);
    if (!dir)
        return rep_stat_only(msg, -(i32)E_NOTDIR);

    fs_inode_s* inode = NULL;
    ret = fs_driver->mkdir(dir, req->name, req->mode, &inode);
    return rep_entry(msg, ret, inode);
}

static i32 handle_rmdir(ipc_msg_s* msg, u32 badge) {
    fs_name_req_s* req          = (fs_name_req_s*)msg->payload;
    req->name[VFS_NAME_MAX - 1] = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_inode_s* dir = dir_of(req->sb, req->dir);
    if (!dir)
        return rep_stat_only(msg, -(i32)E_NOTDIR);

    return rep_stat_only(msg, fs_driver->rmdir(dir, req->name));
}

static i32 handle_unlink(ipc_msg_s* msg, u32 badge) {
    fs_name_req_s* req          = (fs_name_req_s*)msg->payload;
    req->name[VFS_NAME_MAX - 1] = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_inode_s* dir = dir_of(req->sb, req->dir);
    if (!dir)
        return rep_stat_only(msg, -(i32)E_NOTDIR);

    return rep_stat_only(msg, fs_driver->unlink(dir, req->name));
}

static i32 handle_link(ipc_msg_s* msg, u32 badge) {
    fs_link_req_s* req          = (fs_link_req_s*)msg->payload;
    req->name[VFS_NAME_MAX - 1] = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    fs_inode_s* dir = dir_of(req->sb, req->dir);
    fs_inode_s* inode = sb ? fs_ilookup(sb, req->ino) : NULL;
    if (!dir || !inode)
        return rep_stat_only(msg, -(i32)E_NOENT);

    ret = fs_driver->link(inode, dir, req->name);
    if (ret == E_OK)
        fs_ihold(inode);

    return rep_entry(msg, ret, inode);
}

static i32 handle_symlink(ipc_msg_s* msg, u32 badge) {
    fs_symlink_req_s* req        = (fs_symlink_req_s*)msg->payload;
    req->name[FS_LINK_MAX - 1]   = '\0';
    req->target[FS_LINK_MAX - 1] = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_inode_s* dir = dir_of(req->sb, req->dir);
    if (!dir)
        return rep_stat_only(msg, -(i32)E_NOTDIR);

    fs_inode_s* inode = NULL;
    ret = fs_driver->symlink(dir, req->name, req->target, &inode);
    return rep_entry(msg, ret, inode);
}

static i32 handle_readlink(ipc_msg_s* msg, u32 badge) {
    fs_node_req_s* req = (fs_node_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    fs_inode_s* inode = sb ? fs_ilookup(sb, req->ino) : NULL;
    if (!inode)
        return rep_stat_only(msg, -(i32)E_NOENT);

    if (!VFS_ISLNK(inode->mode))
        return rep_stat_only(msg, -(i32)E_INVAL);

    fs_data_reply_s reply;
    reply.ret = fs_driver->readlink(inode, (char*)reply.data, FS_READ_MAX);
    u32 len = sizeof(reply.ret) + ((reply.ret > 0) ? (u32)reply.ret : 0u);
    memcpy(msg->payload, &reply, len);
    return (i32)len;
}

static i32 handle_rename(ipc_msg_s* msg, u32 badge) {
    fs_rename_req_s* req         = (fs_rename_req_s*)msg->payload;
    req->from[FS_RENAME_MAX - 1] = '\0';
    req->to[FS_RENAME_MAX - 1]   = '\0';

    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_inode_s* from_dir = dir_of(req->sb, req->from_dir);
    fs_inode_s* to_dir = dir_of(req->sb, req->to_dir);
    if (!from_dir || !to_dir)
        return rep_stat_only(msg, -(i32)E_NOTDIR);

    return rep_stat_only(msg, fs_driver->rename(from_dir, req->from, to_dir, req->to));
}

static i32 handle_truncate(ipc_msg_s* msg, u32 badge) {
    fs_trunc_req_s* req = (fs_trunc_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    fs_inode_s* inode = sb ? fs_ilookup(sb, req->ino) : NULL;
    if (!inode)
        return rep_stat_only(msg, -(i32)E_NOENT);

    if (!VFS_ISREG(inode->mode))
        return rep_stat_only(msg, -(i32)E_INVAL);

    return rep_stat_only(msg, fs_driver->truncate(inode, req->length));
}

static i32 handle_read(ipc_msg_s* msg, u32 badge) {
    fs_read_req_s* req = (fs_read_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    fs_inode_s* inode = sb ? fs_ilookup(sb, req->ino) : NULL;
    if (!inode)
        return rep_stat_only(msg, -(i32)E_NOENT);

    if (!VFS_ISREG(inode->mode))
        return rep_stat_only(msg, -(i32)E_INVAL);

    fs_data_reply_s reply;
    reply.ret = fs_driver->read(inode, reply.data, min(req->count, (u32)FS_READ_MAX), req->offset);
    u32 len = sizeof(reply.ret) + ((reply.ret > 0) ? (u32)reply.ret : 0u);
    memcpy(msg->payload, &reply, len);
    return (i32)len;
}

static i32 handle_write(ipc_msg_s* msg, u32 badge) {
    fs_write_req_s* req = (fs_write_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_super_s* sb = fs_super_find(req->sb);
    fs_inode_s* inode = sb ? fs_ilookup(sb, req->ino) : NULL;
    if (!inode)
        return rep_stat_only(msg, -(i32)E_NOENT);

    if (!VFS_ISREG(inode->mode))
        return rep_stat_only(msg, -(i32)E_INVAL);

    i64 offset = (req->flags & FS_WRITE_APPEND) ? inode->size : req->offset;

    fs_write_reply_s reply = {0};
    reply.ret              = fs_driver->write(inode, req->data, min((u32)req->count, (u32)FS_WRITE_MAX), offset);
    reply.pos              = offset + ((reply.ret > 0) ? reply.ret : 0);
    memcpy(msg->payload, &reply, sizeof(reply));
    return (i32)sizeof(reply);
}

static i32 handle_readdir(ipc_msg_s* msg, u32 badge) {
    fs_readdir_req_s* req = (fs_readdir_req_s*)msg->payload;
    i32 ret = check_badge(badge);
    if (ret != E_OK)
        return rep_stat_only(msg, ret);

    fs_inode_s* dir = dir_of(req->sb, req->ino);
    if (!dir)
        return rep_stat_only(msg, -(i32)E_NOTDIR);

    fs_readdir_reply_s reply = {0};
    reply.ret                = fs_driver->readdir(dir, req->cookie, &reply);
    if (reply.ret < 0)
        return rep_stat_only(msg, reply.ret);

    memcpy(msg->payload, &reply, sizeof(reply));
    return (i32)sizeof(reply);
}

static i32 init(void) {
    heap_init();
    fs_icache_init();
    fs_super_init();

    i32 ret = fs_driver->init ? fs_driver->init() : E_OK;
    if (ret != E_OK)
        return ret;

    printf("[%s] Initialized userspace %s server\n", fs_driver->name, fs_driver->name);
    return E_OK;
}

static void fini(void) {
    ;
}

void fs_server_main(const fs_driver_s* driver) {
    static server_s server = {
        .init     = init,
        .fini     = fini,
        .handlers = fs_handlers,
    };

    fs_driver = driver;
    server.name = (char*)driver->name;
    run(&server);
}
