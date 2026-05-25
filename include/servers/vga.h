#pragma once

#include <stddef.h>

#include "types.h"
#include "uapi.h"

#define VGA_SERVER_NAME "vga"

#define VGA_SERVER_OPS(X) \
    X(1, putc)            \
    X(2, puts)            \
    X(3, write)           \
    X(4, clear)

typedef enum vga_server_op_t {
#define VGA_SERVER_OP_ENUM(id, name) VGA_SERVER_OP_##name = (id),
    VGA_SERVER_OPS(VGA_SERVER_OP_ENUM)
#undef VGA_SERVER_OP_ENUM
} vga_server_op_t;

typedef struct vga_server_req_t {
    u8   len;                              /* Buffer length                      */
    char buf[IPC_PAYLOAD_SZ - sizeof(u8)]; /* Character, string, or write buffer */
} vga_server_req_t;

_Static_assert(sizeof(vga_server_req_t) == IPC_PAYLOAD_SZ, "Error: vga_server_req_t size is not IPC_PAYLOAD_SZ");
