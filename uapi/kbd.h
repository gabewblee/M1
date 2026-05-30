#pragma once

#include "uapi/ipc.h"
#include "uapi/types.h"

#define KBD_SERVER_NAME "keyboard"

#define KBD_SERVER_OPS(X) \
    X(1, read)

enum kbd_server_op_e {
#define KBD_SERVER_OP_ENUM(id, name) KBD_SERVER_OP_##name = (id),
    KBD_SERVER_OPS(KBD_SERVER_OP_ENUM)
#undef KBD_SERVER_OP_ENUM
};

typedef struct kbd_event_s {
    u8 code;     /* Key code w/o break bit      */
    u8 extended; /* Whether the key is extended */
    u8 pressed;  /* Whether the key is pressed  */
} kbd_event_s;

typedef struct kbd_server_reply_s {
    i32         ret;   /* Operation status */
    kbd_event_s event; /* Keyboard event   */
} kbd_server_reply_s;

_Static_assert(sizeof(kbd_server_reply_s) <= IPC_PAYLOAD_SZ, "Error: Invalid kbd_server_reply_s size");
