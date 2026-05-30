#pragma once

#include "uapi/uapi.h"

typedef struct server_s {
    const char* name;                        /* Server name             */
    i32         (*init)(void);               /* Initialization function */
    i32         (*dispatch)(ipc_msg_s* msg); /* Dispatch function       */
    void        (*fini)(void);               /* Finalization function   */
} server_s;

/**
 * run - Runs @server. Initializes the server, receives IPC messages,
 *       dispatches them, and replies to the sender.
 * @server: The server to run.
 *
 * Context: All servers must implement init(), dispatch(), and fini(),
 *          which this function calls in a uniform IPC loop.
 */
void run(const server_s* server);