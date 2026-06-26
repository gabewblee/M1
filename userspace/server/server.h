#pragma once

#include <uapi/uapi.h>

#define SERVER_OP_MAX             64u
#define SERVER_OP_DECL(id, name)  static i32 handle_##name(ipc_msg_s* msg);
#define SERVER_OP_ENTRY(id, name) [id] = handle_##name,

typedef i32 (*server_handler_f)(ipc_msg_s* msg);

typedef struct server_s {
    char*                   name;          /* Server name             */
    i32                     (*init)(void); /* Initialization function */
    void                    (*fini)(void); /* Finalization function   */
    const server_handler_f* handlers;      /* Operation table         */
} server_s;

/**
 * rep_stat_only - Fills @msg with @status.
 * @msg: The IPC message.
 * @status: The status code.
 * Returns: @status.
 */
i32 rep_stat_only(ipc_msg_s* msg, i32 status);

/**
 * run - Runs @server. Initializes the server, receives IPC messages,
 *       dispatches them to the registered handlers, and replies to the sender.
 * @server: The server to run.
 */
void run(server_s* server);
