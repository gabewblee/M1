#pragma once

#include <stddef.h>
#include <types.h>

#define IPC_PAYLOAD_SZ 56u                                 /* IPC payload size */
#define IPC_MSG_SZ     (sizeof(u32) * 3u + IPC_PAYLOAD_SZ) /* IPC message size */

typedef struct ipc_msg_s {
    u32 id;                   /* Message ID         */
    u32 sender;               /* Sender's thread ID */
    u32 sz;                   /* Payload size       */
    u8  data[IPC_PAYLOAD_SZ]; /* Payload data       */
} ipc_msg_s;

_Static_assert(offsetof(ipc_msg_s, id)     == 0,                "Error: Invalid IPC ID offset");
_Static_assert(offsetof(ipc_msg_s, sender) == sizeof(u32),      "Error: Invalid IPC sender offset");
_Static_assert(offsetof(ipc_msg_s, sz)     == sizeof(u32) * 2u, "Error: Invalid IPC size offset");
_Static_assert(offsetof(ipc_msg_s, data)   == sizeof(u32) * 3u, "Error: Invalid IPC data offset");
_Static_assert(sizeof(ipc_msg_s)           == IPC_MSG_SZ,       "Error: Invalid IPC message size");
