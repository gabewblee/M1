#pragma once

#include <ipc.h>
#include <servers.h>
#include <types.h>

#define ROOT_SERVER_OPS(X) \
    X(1, spawn)

typedef enum root_server_op_e {
#define ROOT_SERVER_OP_ENUM(id, name) ROOT_SERVER_OP_##name = (id),
    ROOT_SERVER_OPS(ROOT_SERVER_OP_ENUM)
#undef ROOT_SERVER_OP_ENUM
} root_server_op_e;

#define EXEC_BADGE       (SERVER_ID_CNT + 1u) /* Badge minted into spawned processes    */
#define EXEC_CMDLINE_MAX IPC_PAYLOAD_SZ       /* Command line bytes, incl. NUL          */
#define EXEC_ARG_MAX     8u                   /* Arguments per command, incl. name      */
#define EXEC_ARGV_VADDR  0xBFFF0000u          /* Command line page virtual address      */

typedef struct root_spawn_req_s {
    char cmdline[EXEC_CMDLINE_MAX]; /* NUL-terminated command line */
} root_spawn_req_s;

_Static_assert(sizeof(root_spawn_req_s) <= IPC_PAYLOAD_SZ, "Error: Invalid root_spawn_req_s size");
