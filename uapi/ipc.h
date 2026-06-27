#pragma once

#include <stddef.h>
#include <types.h>

#define IPC_PAYLOAD_SZ 56u                                 /* IPC payload size */
#define IPC_PACKET_SZ  (sizeof(u32) * 3u + IPC_PAYLOAD_SZ) /* IPC packet size  */

typedef struct ipc_packet_hdr_s {
    u32 op;     /* Operation code     */
    u32 sender; /* Sender's thread ID */
    u32 sz;     /* Payload size       */
} ipc_packet_hdr_s;

typedef struct ipc_packet_s {
    ipc_packet_hdr_s hdr;                     /* Packet header */
    u8               payload[IPC_PAYLOAD_SZ]; /* Payload data  */
} ipc_packet_s;

_Static_assert(offsetof(ipc_packet_s, hdr.op)     == 0,                "Error: Invalid IPC op offset");
_Static_assert(offsetof(ipc_packet_s, hdr.sender) == sizeof(u32),      "Error: Invalid IPC sender offset");
_Static_assert(offsetof(ipc_packet_s, hdr.sz)     == sizeof(u32) * 2u, "Error: Invalid IPC size offset");
_Static_assert(offsetof(ipc_packet_s, payload)    == sizeof(u32) * 3u, "Error: Invalid IPC payload offset");
_Static_assert(sizeof(ipc_packet_s)               == IPC_PACKET_SZ,    "Error: Invalid IPC packet size");
