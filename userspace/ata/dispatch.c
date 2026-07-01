#include <uapi/ata.h>
#include <uapi/servers.h>
#include <userspace/ata/dispatch.h>
#include <userspace/ata/driver.h>
#include <userspace/ata/pio/pio.h>
#include <userspace/libc/capability.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>

ATA_SERVER_OPS(SERVER_OP_DECL)

const server_handler_f ata_handlers[SERVER_OP_MAX] = {
    ATA_SERVER_OPS(SERVER_OP_ENTRY)
};

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

static void print_drv_info(u32 idx, ata_drv_s* drv) {
    printf("Drive %u:\n",               idx);
    printf("  Base I/O port: %x\n",     drv->channel->io);
    printf("  Base control port: %x\n", drv->channel->ctrl);
    printf("  Kind: %s\n",              print_drv_kind(drv->kind));
    printf("  LBA48 support: %s\n",     drv->lba48 ? "Yes" : "No");
    printf("  Slave: %s\n",             drv->slave ? "Yes" : "No");
    printf("  Total sectors: %d\n",     drv->lba48 ? drv->total48 : drv->total28);
}

static i32 handle_info(ipc_packet_s* packet) {
    ata_server_reply_s rep = {0};
    ata_server_req_s* req = (ata_server_req_s*)packet->payload;

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

    memcpy(packet->payload, &rep, sizeof(rep));
    return (i32)sizeof(rep);
}

static i32 handle_read(ipc_packet_s* packet) {
    ata_server_req_s* req = (ata_server_req_s*)packet->payload;
    ata_server_reply_s rep = {0};

    ata_drv_s* drv = ata_drv_lookup(req->drv);
    if (!drv || !drv->present) {
        rep.ret = -(i32)E_INVAL;
    } else {
        rep.ret  = ata_pio_read(drv, req->lba, req->cnt, (void*)req->buf);
        rep.done = (rep.ret == E_OK) ? req->cnt : 0;
    }

    memcpy(packet->payload, &rep, sizeof(rep));
    return (i32)sizeof(rep);
}

static i32 handle_write(ipc_packet_s* packet) {
    ata_server_req_s* req = (ata_server_req_s*)packet->payload;
    ata_server_reply_s rep = {0};

    ata_drv_s* drv = ata_drv_lookup(req->drv);
    if (!drv || !drv->present) {
        rep.ret = -(i32)E_INVAL;
    } else {
        rep.ret  = ata_pio_write(drv, req->lba, req->cnt, (void*)req->buf);
        rep.done = (rep.ret == E_OK) ? req->cnt : 0;
    }

    memcpy(packet->payload, &rep, sizeof(rep));
    return (i32)sizeof(rep);
}

i32 init(void) {
    /* Bind both ATA channel IRQ handlers to our single device notification, so
       sys_wait wakes us when either bus completes a transfer. */
    i32 ret = irq_handler_set_notification(SERVICE_CPTR_IRQ, SERVICE_CPTR_NTFN, SERVICE_CNODE_DEPTH);
    if (ret != E_OK)
        return ret;

    ret = irq_handler_set_notification(SERVICE_CPTR_IRQ2, SERVICE_CPTR_NTFN, SERVICE_CNODE_DEPTH);
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

void fini(void) {
    ;
}
