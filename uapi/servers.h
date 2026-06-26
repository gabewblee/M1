#pragma once

#include <types.h>

/* X(id, name) */
#define SERVERS(X) \
    X(0, vga)      \
    X(1, keyboard) \
    X(2, ata)

typedef enum server_id_e {
#define SERVER_ID_ENUM(id, name) SERVER_ID_##name = (id),
    SERVERS(SERVER_ID_ENUM)
#undef SERVER_ID_ENUM
    SERVER_ID_CNT,
} server_id_e;

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
