#pragma once

#include <stddef.h>

#include "uapi/ipc.h"
#include "uapi/types.h"

#define VGA_SERVER_NAME "vga"

#define VGA_SERVER_OPS(X) \
    X(1, putc)            \
    X(2, puts)            \
    X(3, write)           \
    X(4, clear)

enum vga_server_op_e {
#define VGA_SERVER_OP_ENUM(id, name) VGA_SERVER_OP_##name = (id),
    VGA_SERVER_OPS(VGA_SERVER_OP_ENUM)
#undef VGA_SERVER_OP_ENUM
};

typedef struct vga_server_req_s {
    u8   len;                              /* Buffer length                      */
    char buf[IPC_PAYLOAD_SZ - sizeof(u8)]; /* Character, string, or write buffer */
} vga_server_req_s;

_Static_assert(sizeof(vga_server_req_s) == IPC_PAYLOAD_SZ, "Error: Invalid vga_server_req_s size");
