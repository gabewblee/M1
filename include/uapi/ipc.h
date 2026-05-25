#pragma once

#include <stddef.h>

#include "types.h"

#define IPC_MSG_SZ     (sizeof(u32) * 3u + IPC_PAYLOAD_SZ) /* IPC message size */
#define IPC_PAYLOAD_SZ 56u                                 /* IPC payload size */

typedef struct ipc_msg_t {
    u32 id;                   /* Message ID          */
    u32 sender;               /* Sender's thread ID  */
    u32 sz;                   /* Payload size        */
    u8  data[IPC_PAYLOAD_SZ]; /* Inline payload data */
} ipc_msg_t;

_Static_assert(offsetof(ipc_msg_t, id) == 0, "Error: Invalid IPC ID offset");
_Static_assert(offsetof(ipc_msg_t, sender) == sizeof(u32), "Error: Invalid IPC sender offset");
_Static_assert(offsetof(ipc_msg_t, sz) == sizeof(u32) * 2u, "Error: Invalid IPC size offset");
_Static_assert(offsetof(ipc_msg_t, data) == sizeof(u32) * 3u, "Error: Invalid IPC data offset");
_Static_assert(sizeof(ipc_msg_t) == IPC_MSG_SZ, "Error: Invalid IPC message size");