#include "driver.h"
#include "dispatch.h"
#include "pio/pio.h"
#include "uapi/ata.h"

#include "../libc/stdio.h"
#include "../libc/string.h"
#include "../libc/syscall.h"

#define ATA_PRIMARY_BUS_IRQ   14 /* ATA primary bus IRQ number   */
#define ATA_SECONDARY_BUS_IRQ 15 /* ATA secondary bus IRQ number */

static char* print_drv_kind(u32 kind) {
    switch (kind) {
        case ATA_KIND_NONE:
            return "NONE";
        case ATA_KIND_PATA:
            return "PATA";
        case ATA_KIND_PATAPI:
            return "PATAPI";
        case ATA_KIND_SATA:
            return "SATA";
        case ATA_KIND_SATAPI:
            return "SATAPI";
        default:
            return "UNKNOWN";
    }
}

static void print_drv_info(u32 idx, const ata_drv_s* drv) {
    printf("Drive %u:\n",               idx);
    printf("  Base I/O port: %x\n",     drv->channel->io);
    printf("  Base control port: %x\n", drv->channel->ctrl);
    printf("  Kind: %s\n",              print_drv_kind(drv->kind));
    printf("  LBA48 support: %s\n",     drv->lba48 ? "Yes" : "No");
    printf("  Slave: %s\n",             drv->slave ? "Yes" : "No");
    printf("  Total sectors: %d\n",     drv->lba48 ? drv->total48 : drv->total28);
}

static i32 handle_info(ipc_msg_s* msg) {
    ata_server_reply_s rep = {0};
    ata_server_req_s* req = (ata_server_req_s*)msg->data;

    ata_drv_s* drv = ata_drv_lookup(req->drv);
    if (!drv) {
        rep.ret = -(i32)E_INVAL;
    } else {
        rep.ret          = E_OK;
        rep.info.present = drv->present;
        rep.info.kind    = drv->kind;
        rep.info.lba48   = drv->lba48;
        rep.info.slave   = drv->slave;
        rep.info.sectors = drv->lba48 ? drv->total48 : drv->total28;
    }

    msg->sz = (u32)sizeof(rep);
    memcpy(msg->data, &rep, sizeof(rep));
    return rep.ret;
}

static i32 handle_read(ipc_msg_s* msg) {
    ata_server_req_s* req = (ata_server_req_s*)msg->data;
    ata_server_reply_s rep = {0};

    ata_drv_s* drv = ata_drv_lookup(req->drv);
    if (!drv || !drv->present) {
        rep.ret = -(i32)E_INVAL;
    } else {
        rep.ret  = ata_pio_read(drv, req->lba, req->cnt, (void*)req->buf);
        rep.done = (rep.ret == E_OK) ? req->cnt : 0;
    }

    msg->sz = (u32)sizeof(rep);
    memcpy(msg->data, &rep, sizeof(rep));
    return rep.ret;
}

static i32 handle_write(ipc_msg_s* msg) {
    ata_server_req_s* req = (ata_server_req_s*)msg->data;
    ata_server_reply_s rep = {0};

    ata_drv_s* drv = ata_drv_lookup(req->drv);
    if (!drv || !drv->present) {
        rep.ret = -(i32)E_INVAL;
    } else {
        rep.ret  = ata_pio_write(drv, req->lba, req->cnt, (void*)req->buf);
        rep.done = (rep.ret == E_OK) ? req->cnt : 0;
    }

    msg->sz = (u32)sizeof(rep);
    memcpy(msg->data, &rep, sizeof(rep));
    return rep.ret;
}

i32 init(void) {
    i32 ret = sys_irq_register_handler(ATA_PRIMARY_BUS_IRQ);
    if (ret != E_OK)
        return ret;

    ret = sys_irq_register_handler(ATA_SECONDARY_BUS_IRQ);
    if (ret != E_OK)
        return ret;

    ata_drvs_init();
    printf("[ATA] Initialized userspace ATA PIO server\n");
    printf("[ATA] Detected %u drive(s):\n", ata_get_present_drv_cnt());
    for (u32 idx = 0; idx < 4; idx++) {
        ata_drv_s* drv = ata_drv_lookup(idx);
        if (drv && drv->present)
            print_drv_info(idx, drv);
    }
    return E_OK;
}

i32 dispatch(ipc_msg_s* msg) {
    switch ((enum ata_server_op_e)msg->id) {
        case ATA_SERVER_OP_info:
            return handle_info(msg);
        case ATA_SERVER_OP_read:
            return handle_read(msg);
        case ATA_SERVER_OP_write:
            return handle_write(msg);
        default:
            i32 ret = -(i32)E_NOSYS;
            msg->sz = (u32)sizeof(i32);
            memcpy(msg->data, &ret, sizeof(ret));
            return ret;
    }
}

void fini(void) {
    ;
}