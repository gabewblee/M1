#pragma once

#include <types.h>

typedef enum server_id_e {
    SERVER_ID_vga      = 0,
    SERVER_ID_keyboard = 1,
    SERVER_ID_ata      = 2,
    SERVER_ID_CNT      = 3,
} server_id_e;

#define SERVICE_CNODE_RADIX 4u /* Server CSpace slot count log2 (16 slots)    */
#define SERVICE_CNODE_DEPTH 4u /* Bits resolved per server cptr               */
#define SERVICE_CPTR_NULL   0u /* Reserved null slot                          */
#define SERVICE_CPTR_EP     1u /* Endpoint the server receives requests on    */
#define SERVICE_CPTR_REPLY  2u /* Reply object for Recv / ReplyRecv           */
#define SERVICE_CPTR_VSPACE 3u /* The server's own VSpace (page directory)    */
#define SERVICE_CPTR_VGA    4u /* Endpoint a client calls the VGA server on   */
#define SERVICE_CPTR_VGA_FB 4u /* VGA server's framebuffer device frame       */
#define SERVICE_CPTR_NTFN   5u /* Notification a device server waits IRQs on  */
#define SERVICE_CPTR_IRQ    6u /* Primary IRQ handler for a device server     */
#define SERVICE_CPTR_IRQ2   7u /* Secondary IRQ handler (ATA second channel)  */

#define SERVER_NOTE_TYPE 0x4D315256u /* "M1RV" */

typedef struct server_desc_s {
    u32 id;       /* Server ID           */
    u8  priority; /* Scheduler priority  */
    u8  iopl;     /* I/O privilege level */
    u8  _pad[2];  /* Reserved            */
} server_desc_s;

#define SERVER_DEF(_id, _prio, _iopl)                                      \
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
        .desc     = { .id = (_id), .priority = (_prio), .iopl = (_iopl) }, \
    }
