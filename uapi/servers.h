#pragma once

#include <types.h>

typedef enum server_id_e {
    SERVER_ID_vga      = 0, /* VGA server ID      */
    SERVER_ID_keyboard = 1, /* PS/2 kbd server ID */
    SERVER_ID_ata      = 2, /* ATA server ID      */
    SERVER_ID_CNT      = 3, /* Server count       */
} server_id_e;

#define SERVICE_CNODE_RADIX 4u          /* log2(server CSpace slot count) */
#define SERVICE_CNODE_DEPTH 4u          /* Server cptr bits               */
#define SERVICE_CPTR_NULL   0u          /* Null slot                      */
#define SERVICE_CPTR_EP     1u          /* Endpoint slot                  */
#define SERVICE_CPTR_REPLY  2u          /* Reply slot                     */
#define SERVICE_CPTR_VSPACE 3u          /* Server's VSpace                */
#define SERVICE_CPTR_VGA    4u          /* VGA server's endpoint          */
#define SERVICE_CPTR_VGA_FB 4u          /* VGA server's device frame      */
#define SERVICE_CPTR_NTFN   5u          /* Notification slot              */
#define SERVICE_CPTR_IRQ    6u          /* Primary IRQ handler slot       */
#define SERVICE_CPTR_IRQ2   7u          /* Secondary IRQ handler slot     */

#define SERVER_NOTE_TYPE    0x4D315256u /* "M1RV"                         */

typedef enum resource_type_e {
    RESOURCE_TYPE_NONE    = 0, /* Resource list termination sentinel */
    RESOURCE_TYPE_IRQ     = 1, /* arg = IRQ line                     */
    RESOURCE_TYPE_NTFN    = 2, /* arg = Unused                       */
    RESOURCE_TYPE_DEV_FRM = 3, /* arg = Device untyped boot slot     */
    RESOURCE_TYPE_SERV_EP = 4, /* arg = Server ID                    */
} resource_type_e;

typedef struct resource_s {
    u32 type; /* One of RESOURCE_TYPE_* */
    u32 slot; /* One of SERVICE_CPTR_*  */
    u32 arg;  /* Type-specific argument */
} resource_s;

#define SERVER_MAX_RES_CNT 4u /* Maximum resource count */

typedef struct server_desc_s {
    u32        id;                      /* Server ID                  */
    u8         priority;                /* Server priority            */
    u8         iopl;                    /* Server I/O privilege level */
    u8         _pad[2];                 /* Reserved                   */
    resource_s res[SERVER_MAX_RES_CNT]; /* Server resources           */
} server_desc_s;

#define RES_IRQ(_slot, _irq)          { .type = RESOURCE_TYPE_IRQ,     .slot = (_slot), .arg = (_irq)      }
#define RES_NTFN(_slot)               { .type = RESOURCE_TYPE_NTFN,    .slot = (_slot)                     }
#define RES_DEV_FRM(_slot, _bootslot) { .type = RESOURCE_TYPE_DEV_FRM, .slot = (_slot), .arg = (_bootslot) }
#define RES_SERV_EP(_slot, _server)   { .type = RESOURCE_TYPE_SERV_EP, .slot = (_slot), .arg = (_server)   }

#define SERVER_DEF(_id, _prio, _iopl, ...)                                 \
    static __attribute__((__section__(".note"), __used__, __aligned__(4))) \
    const struct {                                                         \
        u32           n_namesz;                                            \
        u32           n_descsz;                                            \
        u32           n_type;                                              \
        char          name[4];                                             \
        server_desc_s desc;                                                \
    } __server_note = {                                                    \
        .n_namesz = 3u,                                                    \
        .n_descsz = sizeof(server_desc_s),                                 \
        .n_type   = SERVER_NOTE_TYPE,                                      \
        .name     = "M1",                                                  \
        .desc     = {                                                      \
            .id       = (_id),                                             \
            .priority = (_prio),                                           \
            .iopl     = (_iopl),                                           \
            .res      = { __VA_ARGS__ }                                    \
        }                                                                  \
    }
