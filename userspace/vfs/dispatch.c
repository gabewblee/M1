#include <userspace/libc/minmax.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/vfs/dcache.h>
#include <userspace/vfs/dispatch.h>
#include <userspace/vfs/file.h>
#include <userspace/vfs/heap.h>
#include <userspace/vfs/inode.h>
#include <userspace/vfs/log.h>
#include <userspace/vfs/mount.h>
#include <userspace/vfs/namei.h>
#include <userspace/vfs/radix.h>
#include <userspace/vfs/ramfs.h>
#include <userspace/vfs/selftest.h>
#include <userspace/vfs/super.h>

VFS_SERVER_OPS(SERVER_OP_DECL)

const server_handler_f vfs_handlers[SERVER_OP_MAX] = {
    VFS_SERVER_OPS(SERVER_OP_ENTRY)
};

static i32 client_of(u32 badge) {
    return (badge >= 1u && badge < VFS_CLIENT_MAX) ? (i32)badge : -(i32)E_PERM;
}

static i32 handle_open(ipc_msg_s* msg, u32 badge) {
    vfs_path_req_s* req = (vfs_path_req_s*)msg->payload;
    req->path[VFS_PATH_MAX - 1] = '\0';

    i32 client = client_of(badge);
    if (client < 0)
        return rep_stat_only(msg, client);

    return rep_stat_only(msg, fd_open((u32)client, req->path, req->flags, req->mode));
}

static i32 handle_close(ipc_msg_s* msg, u32 badge) {
    vfs_file_req_s* req = (vfs_file_req_s*)msg->payload;
    i32 client = client_of(badge);
    if (client < 0)
        return rep_stat_only(msg, client);

    return rep_stat_only(msg, fd_close((u32)client, req->fd));
}

static i32 handle_read(ipc_msg_s* msg, u32 badge) {
    vfs_file_req_s* req = (vfs_file_req_s*)msg->payload;
    i32 client = client_of(badge);

    vfs_data_reply_s rep;
    rep.ret = (client < 0)
        ? client
        : fd_read((u32)client, req->fd, rep.data, min(req->count, (u32)VFS_READ_MAX));

    u32 len = sizeof(rep.ret) + ((rep.ret > 0) ? (u32)rep.ret : 0u);
    memcpy(msg->payload, &rep, len);
    return (i32)len;
}

static i32 handle_write(ipc_msg_s* msg, u32 badge) {
    vfs_write_req_s* req = (vfs_write_req_s*)msg->payload;
    i32 client = client_of(badge);
    if (client < 0)
        return rep_stat_only(msg, client);

    return rep_stat_only(msg, fd_write((u32)client, req->fd, req->data, min(req->count, (u32)VFS_WRITE_MAX)));
}

static i32 handle_lseek(ipc_msg_s* msg, u32 badge) {
    vfs_file_req_s* req = (vfs_file_req_s*)msg->payload;
    i32 client = client_of(badge);

    vfs_seek_reply_s rep = {0};
    rep.ret = (client < 0)
        ? client
        : fd_seek((u32)client, req->fd, req->offset, req->whence, &rep.offset);

    memcpy(msg->payload, &rep, sizeof(rep));
    return (i32)sizeof(rep);
}

static i32 handle_stat(ipc_msg_s* msg, u32 badge) {
    vfs_path_req_s* req = (vfs_path_req_s*)msg->payload;
    req->path[VFS_PATH_MAX - 1] = '\0';

    vfs_stat_reply_s rep = {0};
    rep.ret = do_stat(req->path, req->flags, &rep.stat);
    memcpy(msg->payload, &rep, sizeof(rep));
    return (i32)sizeof(rep);
}

static i32 handle_fstat(ipc_msg_s* msg, u32 badge) {
    vfs_file_req_s* req = (vfs_file_req_s*)msg->payload;
    i32 client = client_of(badge);

    vfs_stat_reply_s rep = {0};
    rep.ret = (client < 0) ? client : fd_stat((u32)client, req->fd, &rep.stat);
    memcpy(msg->payload, &rep, sizeof(rep));
    return (i32)sizeof(rep);
}

static i32 handle_mkdir(ipc_msg_s* msg, u32 badge) {
    vfs_path_req_s* req = (vfs_path_req_s*)msg->payload;
    req->path[VFS_PATH_MAX - 1] = '\0';
    return rep_stat_only(msg, do_mkdir(req->path, req->mode));
}

static i32 handle_rmdir(ipc_msg_s* msg, u32 badge) {
    vfs_path_req_s* req = (vfs_path_req_s*)msg->payload;
    req->path[VFS_PATH_MAX - 1] = '\0';
    return rep_stat_only(msg, do_rmdir(req->path));
}

static i32 handle_unlink(ipc_msg_s* msg, u32 badge) {
    vfs_path_req_s* req = (vfs_path_req_s*)msg->payload;
    req->path[VFS_PATH_MAX - 1] = '\0';
    return rep_stat_only(msg, do_unlink(req->path));
}

static i32 handle_link(ipc_msg_s* msg, u32 badge) {
    vfs_pair_req_s* req = (vfs_pair_req_s*)msg->payload;
    req->from[VFS_PAIR_MAX - 1] = '\0';
    req->to[VFS_PAIR_MAX - 1]   = '\0';
    return rep_stat_only(msg, do_link(req->from, req->to));
}

static i32 handle_symlink(ipc_msg_s* msg, u32 badge) {
    vfs_pair_req_s* req = (vfs_pair_req_s*)msg->payload;
    req->from[VFS_PAIR_MAX - 1] = '\0';
    req->to[VFS_PAIR_MAX - 1]   = '\0';
    return rep_stat_only(msg, do_symlink(req->from, req->to));
}

static i32 handle_readlink(ipc_msg_s* msg, u32 badge) {
    vfs_path_req_s* req = (vfs_path_req_s*)msg->payload;
    req->path[VFS_PATH_MAX - 1] = '\0';

    vfs_data_reply_s rep;
    rep.ret = do_readlink(req->path, (char*)rep.data, VFS_READ_MAX);
    u32 len = sizeof(rep.ret) + ((rep.ret > 0) ? (u32)rep.ret : 0u);
    memcpy(msg->payload, &rep, len);
    return (i32)len;
}

static i32 handle_rename(ipc_msg_s* msg, u32 badge) {
    vfs_pair_req_s* req = (vfs_pair_req_s*)msg->payload;
    req->from[VFS_PAIR_MAX - 1] = '\0';
    req->to[VFS_PAIR_MAX - 1]   = '\0';
    return rep_stat_only(msg, do_rename(req->from, req->to));
}

static i32 handle_readdir(ipc_msg_s* msg, u32 badge) {
    vfs_file_req_s* req = (vfs_file_req_s*)msg->payload;
    i32 client = client_of(badge);

    vfs_dirent_reply_s rep = {0};
    rep.ret = (client < 0) ? client : fd_readdir((u32)client, req->fd, &rep.dirent);
    memcpy(msg->payload, &rep, sizeof(rep));
    return (i32)sizeof(rep);
}

static i32 handle_truncate(ipc_msg_s* msg, u32 badge) {
    vfs_trunc_req_s* req = (vfs_trunc_req_s*)msg->payload;
    req->path[VFS_PATH_MAX - 1] = '\0';
    return rep_stat_only(msg, do_truncate(req->path, req->length));
}

static i32 handle_mount(ipc_msg_s* msg, u32 badge) {
    vfs_mount_req_s* req = (vfs_mount_req_s*)msg->payload;
    req->source[VFS_MOUNT_MAX - 1]  = '\0';
    req->target[VFS_MOUNT_MAX - 1]  = '\0';
    req->fstype[VFS_FSNAME_MAX - 1] = '\0';
    return rep_stat_only(msg, do_mount(req->source, req->target, req->fstype, req->flags));
}

static i32 handle_umount(ipc_msg_s* msg, u32 badge) {
    vfs_path_req_s* req = (vfs_path_req_s*)msg->payload;
    req->path[VFS_PATH_MAX - 1] = '\0';
    return rep_stat_only(msg, do_umount(req->path));
}

i32 init(void) {
    heap_init();
    radix_init();
    dcache_init();
    icache_init();
    super_init();
    file_init();
    ramfs_init();

    i32 ret = mount_init("ramfs");
    if (ret != E_OK)
        return ret;

    vfs_log("[VFS] Mounted ramfs root\n");
    vfs_selftest();
    printf("[VFS] Initialized userspace VFS server\n");
    return E_OK;
}

void fini(void) {
    ;
}
