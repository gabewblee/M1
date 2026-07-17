#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/vfs/fsclient.h>

static i32 fs_call(u32 ep, fs_server_op_e op, const void* req, u32 len, void* rep, u32 rep_len) {
    ipc_msg_s msg;
    memcpy(msg.payload, req, len);

    i32 mi = sys_call(ep, mk_msg_info((u32)op, 0, len), &msg, 0);
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

static i32 copy_name(char* dst, u32 size, const char* name, u32 len) {
    if (len >= size)
        return -(i32)E_NAMELONG;

    memcpy(dst, name, len);
    dst[len] = '\0';
    return E_OK;
}

i32 fsc_mount(u32 ep, const char* source, u32 flags, fs_mount_reply_s* reply) {
    fs_mount_req_s req = { .flags = flags };
    if (strlcpy(req.source, source, sizeof(req.source)) >= sizeof(req.source))
        return -(i32)E_NAMELONG;

    return fs_call(ep, FS_SERVER_OP_mount, &req, sizeof(req), reply, sizeof(*reply));
}

i32 fsc_umount(rsb_s* sb) {
    fs_sb_req_s req = { .sb = sb->sb };
    i32 status;
    return fs_call(sb->fstype->ep, FS_SERVER_OP_umount, &req, sizeof(req), &status, sizeof(status));
}

i32 fsc_sync(rsb_s* sb) {
    fs_sb_req_s req = { .sb = sb->sb };
    i32 status;
    return fs_call(sb->fstype->ep, FS_SERVER_OP_sync, &req, sizeof(req), &status, sizeof(status));
}

i32 fsc_statfs(rsb_s* sb, vfs_statfs_s* statfs) {
    fs_sb_req_s req = { .sb = sb->sb };
    fs_statfs_reply_s reply;
    i32 ret = fs_call(sb->fstype->ep, FS_SERVER_OP_statfs, &req, sizeof(req), &reply, sizeof(reply));
    if (ret != E_OK)
        return ret;

    statfs->blocks    = reply.blocks;
    statfs->bfree     = reply.bfree;
    statfs->files     = reply.files;
    statfs->ffree     = reply.ffree;
    statfs->blocksize = reply.blocksize;
    return E_OK;
}

static i32 name_op(fs_server_op_e op, rsb_s* sb, u32 dir, const char* name, u32 len, u32 mode, fs_entry_reply_s* entry) {
    fs_name_req_s req = { .sb = sb->sb, .dir = dir, .mode = mode };
    i32 ret = copy_name(req.name, sizeof(req.name), name, len);
    if (ret != E_OK)
        return ret;

    if (entry)
        return fs_call(sb->fstype->ep, op, &req, sizeof(req), entry, sizeof(*entry));

    i32 status;
    return fs_call(sb->fstype->ep, op, &req, sizeof(req), &status, sizeof(status));
}

i32 fsc_lookup(rsb_s* sb, u32 dir, const char* name, u32 len, fs_entry_reply_s* entry) {
    return name_op(FS_SERVER_OP_lookup, sb, dir, name, len, 0, entry);
}

i32 fsc_forget(rsb_s* sb, u32 ino, u32 count) {
    fs_node_req_s req = { .sb = sb->sb, .ino = ino, .count = count };
    i32 status;
    return fs_call(sb->fstype->ep, FS_SERVER_OP_forget, &req, sizeof(req), &status, sizeof(status));
}

i32 fsc_getattr(rsb_s* sb, u32 ino, fs_entry_reply_s* entry) {
    fs_node_req_s req = { .sb = sb->sb, .ino = ino };
    return fs_call(sb->fstype->ep, FS_SERVER_OP_getattr, &req, sizeof(req), entry, sizeof(*entry));
}

i32 fsc_create(rsb_s* sb, u32 dir, const char* name, u32 len, u32 mode, fs_entry_reply_s* entry) {
    return name_op(FS_SERVER_OP_create, sb, dir, name, len, mode, entry);
}

i32 fsc_mkdir(rsb_s* sb, u32 dir, const char* name, u32 len, u32 mode, fs_entry_reply_s* entry) {
    return name_op(FS_SERVER_OP_mkdir, sb, dir, name, len, mode, entry);
}

i32 fsc_rmdir(rsb_s* sb, u32 dir, const char* name, u32 len) {
    return name_op(FS_SERVER_OP_rmdir, sb, dir, name, len, 0, NULL);
}

i32 fsc_unlink(rsb_s* sb, u32 dir, const char* name, u32 len) {
    return name_op(FS_SERVER_OP_unlink, sb, dir, name, len, 0, NULL);
}

i32 fsc_link(rsb_s* sb, u32 ino, u32 dir, const char* name, u32 len, fs_entry_reply_s* entry) {
    fs_link_req_s req = { .sb = sb->sb, .ino = ino, .dir = dir };
    i32 ret = copy_name(req.name, sizeof(req.name), name, len);
    if (ret != E_OK)
        return ret;

    return fs_call(sb->fstype->ep, FS_SERVER_OP_link, &req, sizeof(req), entry, sizeof(*entry));
}

i32 fsc_symlink(rsb_s* sb, u32 dir, const char* name, u32 len, const char* target, fs_entry_reply_s* entry) {
    fs_symlink_req_s req = { .sb = sb->sb, .dir = dir };
    i32 ret = copy_name(req.name, sizeof(req.name), name, len);
    if (ret != E_OK)
        return ret;

    if (strlcpy(req.target, target, sizeof(req.target)) >= sizeof(req.target))
        return -(i32)E_NAMELONG;

    return fs_call(sb->fstype->ep, FS_SERVER_OP_symlink, &req, sizeof(req), entry, sizeof(*entry));
}

i32 fsc_readlink(rsb_s* sb, u32 ino, char* buffer, u32 size) {
    fs_node_req_s req = { .sb = sb->sb, .ino = ino };
    fs_data_reply_s reply;
    i32 ret = fs_call(sb->fstype->ep, FS_SERVER_OP_readlink, &req, sizeof(req), &reply, sizeof(reply));
    if (ret > 0)
        memcpy(buffer, reply.data, min((u32)ret, size));

    return ret;
}

i32 fsc_rename(rsb_s* sb, u32 from_dir, const qstr_s* from, u32 to_dir, const qstr_s* to) {
    fs_rename_req_s req = { .sb = sb->sb, .from_dir = from_dir, .to_dir = to_dir };
    i32 ret = copy_name(req.from, sizeof(req.from), from->name, from->len);
    if (ret != E_OK)
        return ret;

    ret = copy_name(req.to, sizeof(req.to), to->name, to->len);
    if (ret != E_OK)
        return ret;

    i32 status;
    return fs_call(sb->fstype->ep, FS_SERVER_OP_rename, &req, sizeof(req), &status, sizeof(status));
}

i32 fsc_truncate(rsb_s* sb, u32 ino, i64 length) {
    fs_trunc_req_s req = { .sb = sb->sb, .ino = ino, .length = length };
    i32 status;
    return fs_call(sb->fstype->ep, FS_SERVER_OP_truncate, &req, sizeof(req), &status, sizeof(status));
}

i32 fsc_read(rsb_s* sb, u32 ino, void* buffer, u32 count, i64 offset) {
    fs_read_req_s req = {
        .sb     = sb->sb,
        .ino    = ino,
        .count  = min(count, (u32)FS_READ_MAX),
        .offset = offset,
    };

    fs_data_reply_s reply;
    i32 ret = fs_call(sb->fstype->ep, FS_SERVER_OP_read, &req, sizeof(req), &reply, sizeof(reply));
    if (ret > 0)
        memcpy(buffer, reply.data, (u32)ret);

    return ret;
}

i32 fsc_write(rsb_s* sb, u32 ino, const void* buffer, u32 count, i64 offset, u32 append,
              i64* position) {
    fs_write_req_s req = {
        .sb     = sb->sb,
        .ino    = ino,
        .count  = (u16)min(count, (u32)FS_WRITE_MAX),
        .flags  = append ? (u16)FS_WRITE_APPEND : 0u,
        .offset = offset,
    };

    memcpy(req.data, buffer, req.count);

    fs_write_reply_s reply;
    i32 ret = fs_call(sb->fstype->ep, FS_SERVER_OP_write, &req, sizeof(req), &reply, sizeof(reply));
    if (ret > 0 && position)
        *position = reply.pos;

    return ret;
}

i32 fsc_readdir(rsb_s* sb, u32 ino, u32 cookie, fs_readdir_reply_s* reply) {
    fs_readdir_req_s req = { .sb = sb->sb, .ino = ino, .cookie = cookie };
    i32 ret = fs_call(sb->fstype->ep, FS_SERVER_OP_readdir, &req, sizeof(req), reply, sizeof(*reply));
    if (ret > 0)
        reply->name[VFS_NAME_MAX - 1] = '\0';

    return ret;
}
