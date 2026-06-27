#pragma once

#include <ipc.h>
#include <uapi/types.h>

#define ATA_SERVER_OPS(X) \
    X(1, info)            \
    X(2, read)            \
    X(3, write)

enum ata_server_op_e {
#define ATA_SERVER_OP_ENUM(id, name) ATA_SERVER_OP_##name = (id),
    ATA_SERVER_OPS(ATA_SERVER_OP_ENUM)
#undef ATA_SERVER_OP_ENUM
};

enum ata_kind_e {
    ATA_KIND_NONE   = 0,
    ATA_KIND_PATA   = 1,
    ATA_KIND_PATAPI = 2,
    ATA_KIND_SATA   = 3,
    ATA_KIND_SATAPI = 4
};

typedef struct ata_server_req_s {
    u8  drv;   /* Drive index                     */
    u8  cnt;   /* Sector count                    */
    u16 flags; /* Reserved                        */
    u64 lba;   /* Absolute sector address         */
    u32 buf;   /* Buffer address in caller memory */
} ata_server_req_s;

typedef struct ata_server_info_s {
    u8  present; /* Whether the drive exists              */
    u8  kind;    /* Drive kind                            */
    u8  lba48;   /* Whether LBA48 is supported            */
    u8  slave;   /* Whether the drive is slave on channel */
    u64 sectors; /* Total LBA sectors                     */
} ata_server_info_s;

typedef struct ata_server_reply_s {
    i32               ret;  /* Operation status             */
    u16               done; /* Completed sector count       */
    u16               pad;  /* Reserved                     */
    ata_server_info_s info; /* Drive information for info() */
} ata_server_reply_s;

_Static_assert(sizeof(ata_server_req_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid ata_server_req_s size");
_Static_assert(sizeof(ata_server_reply_s) <= IPC_PAYLOAD_SZ, "Error: Invalid ata_server_reply_s size");
