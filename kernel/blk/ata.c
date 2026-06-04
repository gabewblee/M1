#include "kernel/blk/ata.h"
#include "kernel/blk/blk.h"
#include "kernel/ipc/ipc.h"
#include "kernel/ipc/servers.h"
#include "libk/string.h"
#include "uapi/ata.h"
#include "uapi/compiler.h"
#include "uapi/errno.h"
#include "uapi/types.h"

#define ATA_MAX_DRIVE_CNT 4u

typedef struct ata_blk_priv_s {
    u32 task_id; /* ATA server task ID */
    u8  drv;     /* Drive index        */
} ata_blk_priv_s;

static blk_dev_s      ata_blk_devs[ATA_MAX_DRIVE_CNT];
static ata_blk_priv_s ata_blk_privs[ATA_MAX_DRIVE_CNT];

static const char* ata_blk_names[ATA_MAX_DRIVE_CNT] = {
    "ata0",
    "ata1",
    "ata2",
    "ata3"
};

static i32 ata_blk_call(u32 task_id, ipc_msg_s* msg) {
    i32 ret = ipc_call(task_id, msg);
    if (unlikely(ret != E_OK))
        return ret;

    ata_server_reply_s rep;
    memcpy(&rep, msg->data, sizeof(rep));
    return rep.ret;
}

static i32 ata_blk_read(blk_dev_s* dev, u64 lba, u8 cnt, void* buf) {
    const ata_blk_priv_s* priv = (const ata_blk_priv_s*)dev->priv;

    ata_server_req_s req = {
        .drv   = priv->drv,
        .cnt   = cnt,
        .flags = 0,
        .lba   = lba,
        .buf   = (u32)(uintptr_t)buf,
    };

    ipc_msg_s msg = {
        .id = ATA_SERVER_OP_read,
        .sz = (u32)sizeof(req),
    };
    memcpy(msg.data, &req, sizeof(req));

    return ata_blk_call(priv->task_id, &msg);
}

static i32 ata_blk_write(blk_dev_s* dev, u64 lba, u8 cnt, const void* buf) {
    const ata_blk_priv_s* priv = (const ata_blk_priv_s*)dev->priv;

    ata_server_req_s req = {
        .drv   = priv->drv,
        .cnt   = cnt,
        .flags = 0,
        .lba   = lba,
        .buf   = (u32)(uintptr_t)buf,
    };

    ipc_msg_s msg = {
        .id = ATA_SERVER_OP_write,
        .sz = (u32)sizeof(req),
    };
    memcpy(msg.data, &req, sizeof(req));

    return ata_blk_call(priv->task_id, &msg);
}

static i32 ata_blk_info(blk_dev_s* dev, blk_dev_info_s* out) {
    const ata_blk_priv_s* priv = (const ata_blk_priv_s*)dev->priv;

    ata_server_req_s req = {
        .drv   = priv->drv,
        .cnt   = 0,
        .flags = 0,
        .lba   = 0,
        .buf   = 0,
    };

    ipc_msg_s msg = {
        .id = ATA_SERVER_OP_info,
        .sz = (u32)sizeof(req),
    };
    memcpy(msg.data, &req, sizeof(req));

    i32 ret = ipc_call(priv->task_id, &msg);
    if (unlikely(ret != E_OK))
        return ret;

    ata_server_reply_s rep;
    memcpy(&rep, msg.data, sizeof(rep));
    if (unlikely(rep.ret != E_OK))
        return rep.ret;

    out->sectors = rep.info.sectors;
    out->sz      = BLK_DEV_SECTOR_SZ;
    return E_OK;
}

static const blk_dev_ops_s ata_blk_ops = {
    .read  = ata_blk_read,
    .write = ata_blk_write,
    .info  = ata_blk_info,
};

void __init ata_blk_init(void) {
    i32 task_id = server_lookup(ATA_SERVER_NAME);
    if (unlikely(task_id < 0))
        return;

    for (u8 i = 0; i < ATA_MAX_DRIVE_CNT; i++) {
        ata_server_req_s req = {
            .drv   = i,
            .cnt   = 0,
            .flags = 0,
            .lba   = 0,
            .buf   = 0,
        };

        ipc_msg_s msg = {
            .id = ATA_SERVER_OP_info,
            .sz = (u32)sizeof(req),
        };
        memcpy(msg.data, &req, sizeof(req));

        if (ipc_call((u32)task_id, &msg) != E_OK)
            continue;

        ata_server_reply_s rep;
        memcpy(&rep, msg.data, sizeof(rep));

        if (rep.ret != E_OK || !rep.info.present)
            continue;

        ata_blk_privs[i].task_id = (u32)task_id;
        ata_blk_privs[i].drv     = i;

        ata_blk_devs[i].name = ata_blk_names[i];
        ata_blk_devs[i].ops  = &ata_blk_ops;
        ata_blk_devs[i].priv = &ata_blk_privs[i];

        blk_register_dev(&ata_blk_devs[i]);
    }
}
