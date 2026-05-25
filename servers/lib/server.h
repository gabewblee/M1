#pragma once

#include "uapi.h"

typedef struct server_t {
    const char* name;                        /* Server name             */
    i32         (*init)(void);               /* Initialization function */
    i32         (*dispatch)(ipc_msg_t* msg); /* Dispatch function       */
    void        (*fini)(void);               /* Finalization function   */
} server_t;

/**
 * run - Runs @server. Initializes the server,
 *       receives IPC messages, dispatches them to
 *       the server, and replies to the sender.
 * @server: The server to run.
 *
 * Context: This function serves as a uniform interface for all servers.
 *          interface for all servers. All servers
 *          must implement the init(), dispatch(),
 *          and fini() functions that are called
 *          by this function.
 */
void run(const server_t* server);