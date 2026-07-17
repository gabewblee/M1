#include <uapi/errno.h>
#include <uapi/servers.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/libc/vfs.h>

static u32 vfs_ep = SERVICE_CPTR_VFS;

void vfs_client_init(u32 ep) {
    vfs_ep = ep;
}

static i32 call_vfs_server(vfs_server_op_e op, const void* req, u32 len, void* rep, u32 rep_len) {
    ipc_msg_s msg;
    memcpy(msg.payload, req, len);

    i32 mi = sys_call(vfs_ep, mk_msg_info((u32)op, 0, len), &msg, 0);
    if (mi < 0)
        return mi;

    u32 got = get_msg_len((msg_info_t)mi);
    if (got < sizeof(i32))
        return -(i32)E_FAULT;

    memcpy(rep, msg.payload, min(got, rep_len));

    i32 status;
    memcpy(&status, msg.payload, sizeof(status));
    return status;
}

static i32 call_path_op(vfs_server_op_e op, const char* path, u32 flags, u32 mode) {
    vfs_path_req_s req = { .flags = flags, .mode = mode };
    if (strlcpy(req.path, path, sizeof(req.path)) >= sizeof(req.path))
        return -(i32)E_NAMELONG;

    i32 status;
    return call_vfs_server(op, &req, sizeof(req), &status, sizeof(status));
}

static i32 call_pair_op(vfs_server_op_e op, const char* from, const char* to) {
    vfs_pair_req_s req = {0};
    if (strlcpy(req.from, from, sizeof(req.from)) >= sizeof(req.from))
        return -(i32)E_NAMELONG;

    if (strlcpy(req.to, to, sizeof(req.to)) >= sizeof(req.to))
        return -(i32)E_NAMELONG;

    i32 status;
    return call_vfs_server(op, &req, sizeof(req), &status, sizeof(status));
}

i32 vfs_open(const char* path, u32 flags, u32 mode) {
    return call_path_op(VFS_SERVER_OP_open, path, flags, mode);
}

i32 vfs_close(i32 fd) {
    vfs_file_req_s req = { .fd = fd };
    i32 status;
    return call_vfs_server(VFS_SERVER_OP_close, &req, sizeof(req), &status, sizeof(status));
}

i32 vfs_read(i32 fd, void* buffer, u32 count) {
    u8* out = buffer;
    u32 done = 0;
    while (done < count) {
        vfs_file_req_s req = { .fd = fd, .count = min(count - done, (u32)VFS_READ_MAX) };
        vfs_data_reply_s rep;
        i32 ret = call_vfs_server(VFS_SERVER_OP_read, &req, sizeof(req), &rep, sizeof(rep));
        if (ret < 0)
            return done ? (i32)done : ret;

        if (ret == 0)
            break;

        memcpy(out + done, rep.data, (u32)ret);
        done += (u32)ret;
    }

    return (i32)done;
}

i32 vfs_write(i32 fd, const void* buffer, u32 count) {
    const u8* in = buffer;
    u32 done = 0;
    while (done < count) {
        vfs_write_req_s req = { .fd = fd, .count = min(count - done, (u32)VFS_WRITE_MAX) };
        memcpy(req.data, in + done, req.count);

        i32 status;
        i32 ret = call_vfs_server(VFS_SERVER_OP_write, &req, sizeof(req), &status, sizeof(status));
        if (ret < 0)
            return done ? (i32)done : ret;

        if (ret == 0)
            break;

        done += (u32)ret;
    }

    return (i32)done;
}

i32 vfs_seek(i32 fd, i64 offset, u32 whence, i64* result) {
    vfs_file_req_s req = { .fd = fd, .offset = offset, .whence = whence };
    vfs_seek_reply_s rep;
    i32 ret = call_vfs_server(VFS_SERVER_OP_lseek, &req, sizeof(req), &rep, sizeof(rep));
    if (ret == E_OK && result)
        *result = rep.offset;

    return ret;
}

i32 vfs_stat(const char* path, u32 flags, vfs_stat_s* stat) {
    vfs_path_req_s req = { .flags = flags };
    if (strlcpy(req.path, path, sizeof(req.path)) >= sizeof(req.path))
        return -(i32)E_NAMELONG;

    vfs_stat_reply_s rep;
    i32 ret = call_vfs_server(VFS_SERVER_OP_stat, &req, sizeof(req), &rep, sizeof(rep));
    if (ret == E_OK)
        *stat = rep.stat;

    return ret;
}

i32 vfs_fstat(i32 fd, vfs_stat_s* stat) {
    vfs_file_req_s req = { .fd = fd };
    vfs_stat_reply_s rep;
    i32 ret = call_vfs_server(VFS_SERVER_OP_fstat, &req, sizeof(req), &rep, sizeof(rep));
    if (ret == E_OK)
        *stat = rep.stat;

    return ret;
}

i32 vfs_mkdir(const char* path, u32 mode) {
    return call_path_op(VFS_SERVER_OP_mkdir, path, 0, mode);
}

i32 vfs_rmdir(const char* path) {
    return call_path_op(VFS_SERVER_OP_rmdir, path, 0, 0);
}

i32 vfs_unlink(const char* path) {
    return call_path_op(VFS_SERVER_OP_unlink, path, 0, 0);
}

i32 vfs_link(const char* source, const char* target) {
    return call_pair_op(VFS_SERVER_OP_link, source, target);
}

i32 vfs_symlink(const char* content, const char* target) {
    return call_pair_op(VFS_SERVER_OP_symlink, content, target);
}

i32 vfs_readlink(const char* path, char* buffer, u32 size) {
    vfs_path_req_s req = {0};
    if (strlcpy(req.path, path, sizeof(req.path)) >= sizeof(req.path))
        return -(i32)E_NAMELONG;

    vfs_data_reply_s rep;
    i32 ret = call_vfs_server(VFS_SERVER_OP_readlink, &req, sizeof(req), &rep, sizeof(rep));
    if (ret > 0)
        memcpy(buffer, rep.data, min((u32)ret, size));

    return ret;
}

i32 vfs_rename(const char* source, const char* target) {
    return call_pair_op(VFS_SERVER_OP_rename, source, target);
}

i32 vfs_readdir(i32 fd, vfs_dirent_s* entry) {
    vfs_file_req_s req = { .fd = fd };
    vfs_dirent_reply_s rep;
    i32 ret = call_vfs_server(VFS_SERVER_OP_readdir, &req, sizeof(req), &rep, sizeof(rep));
    if (ret > 0)
        *entry = rep.dirent;

    return ret;
}

i32 vfs_truncate(const char* path, i64 length) {
    vfs_trunc_req_s req = { .length = length };
    if (strlcpy(req.path, path, sizeof(req.path)) >= sizeof(req.path))
        return -(i32)E_NAMELONG;

    i32 status;
    return call_vfs_server(VFS_SERVER_OP_truncate, &req, sizeof(req), &status, sizeof(status));
}

i32 vfs_mount(const char* source, const char* target, const char* fstype, u32 flags) {
    vfs_mount_req_s req = { .flags = flags };
    if (strlcpy(req.source, source, sizeof(req.source)) >= sizeof(req.source) ||
        strlcpy(req.target, target, sizeof(req.target)) >= sizeof(req.target) ||
        strlcpy(req.fstype, fstype, sizeof(req.fstype)) >= sizeof(req.fstype))
        return -(i32)E_NAMELONG;

    i32 status;
    return call_vfs_server(VFS_SERVER_OP_mount, &req, sizeof(req), &status, sizeof(status));
}

i32 vfs_umount(const char* target) {
    return call_path_op(VFS_SERVER_OP_umount, target, 0, 0);
}

i32 vfs_statfs(const char* path, vfs_statfs_s* statfs) {
    vfs_path_req_s req = {0};
    if (strlcpy(req.path, path, sizeof(req.path)) >= sizeof(req.path))
        return -(i32)E_NAMELONG;

    vfs_statfs_reply_s rep;
    i32 ret = call_vfs_server(VFS_SERVER_OP_statfs, &req, sizeof(req), &rep, sizeof(rep));
    if (ret == E_OK)
        *statfs = rep.statfs;

    return ret;
}

i32 vfs_sync(void) {
    vfs_path_req_s req = {0};
    i32 status;
    return call_vfs_server(VFS_SERVER_OP_sync, &req, sizeof(req), &status, sizeof(status));
}
