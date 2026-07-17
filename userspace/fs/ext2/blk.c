#include <uapi/ata.h>
#include <userspace/fs/ext2/blk.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>

#define BLK_WIN_SECTORS (SHM_WIN_SZ / ATA_SECTOR_SZ)

static u8* const window = (u8*)SHM_WIN_VADDR(SHM_WIN_ATA);
static u32       blk_drive;
static u64       blk_capacity;

static i32 ata_call(ata_server_op_e op, ata_server_req_s* req, ata_server_reply_s* reply) {
    ipc_msg_s msg;
    memcpy(msg.payload, req, sizeof(*req));

    i32 mi = sys_call(SERVICE_CPTR_ATA, mk_msg_info((u32)op, 0, sizeof(*req)), &msg, 0);
    if (mi < 0)
        return mi;

    if (get_msg_len((msg_info_t)mi) < sizeof(*reply))
        return -(i32)E_FAULT;

    memcpy(reply, msg.payload, sizeof(*reply));
    return reply->ret;
}

i32 blk_init(u32 drive) {
    ata_server_req_s req = { .drv = (u8)drive };
    ata_server_reply_s reply;
    i32 ret = ata_call(ATA_SERVER_OP_info, &req, &reply);
    if (ret != E_OK)
        return ret;

    if (!reply.info.present)
        return -(i32)E_NODEV;

    blk_drive = drive;
    blk_capacity = reply.info.sectors;
    return E_OK;
}

u64 blk_sectors(void) {
    return blk_capacity;
}

static i32 blk_transfer(ata_server_op_e op, u64 lba, u32 count, u8* buffer) {
    while (count) {
        u32 chunk = min(count, (u32)BLK_WIN_SECTORS);
        u32 bytes = chunk * ATA_SECTOR_SZ;
        if (op == ATA_SERVER_OP_write)
            memcpy(window, buffer, bytes);

        ata_server_req_s req = {
            .drv = (u8)blk_drive,
            .cnt = (u8)chunk,
            .win = SHM_WIN_ATA,
            .off = 0,
            .lba = lba,
        };

        ata_server_reply_s reply;
        i32 ret = ata_call(op, &req, &reply);
        if (ret != E_OK)
            return ret;

        if (reply.done != chunk)
            return -(i32)E_IO;

        if (op == ATA_SERVER_OP_read)
            memcpy(buffer, window, bytes);

        lba += chunk;
        count -= chunk;
        buffer += bytes;
    }

    return E_OK;
}

i32 blk_read(u64 lba, u32 count, void* buffer) {
    if (lba + count > blk_capacity)
        return -(i32)E_INVAL;

    return blk_transfer(ATA_SERVER_OP_read, lba, count, buffer);
}

i32 blk_write(u64 lba, u32 count, const void* buffer) {
    if (lba + count > blk_capacity)
        return -(i32)E_INVAL;

    return blk_transfer(ATA_SERVER_OP_write, lba, count, (u8*)buffer);
}
