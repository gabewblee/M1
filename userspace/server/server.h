#pragma once

#include <uapi/uapi.h>

#define SERVER_OP_MAX             64u
#define SERVER_OP_DECL(id, name)  static i32 handle_##name(ipc_packet_s* packet);
#define SERVER_OP_ENTRY(id, name) [id] = handle_##name,

/* A handler reads its request from @packet->payload and writes the reply back
   into the same buffer. It returns the reply length in bytes, or a negative
   error code; on error the framework replies with that code as the status word.
   The requested operation is the message label, so handlers no longer read it
   from the packet. */
typedef i32 (*server_handler_f)(ipc_packet_s* packet);

typedef struct server_s {
    char*                   name;          /* Server name             */
    i32                     (*init)(void); /* Initialization function */
    void                    (*fini)(void); /* Finalization function   */
    const server_handler_f* handlers;      /* Operation table         */
} server_s;

/**
 * rep_stat_only - Writes @status as the reply's leading status word into @packet.
 * @packet: The IPC packet.
 * @status: The status code.
 * Returns: The reply length in bytes (sizeof(i32)).
 */
i32 rep_stat_only(ipc_packet_s* packet, i32 status);

/**
 * run - Runs @server: initializes it, then loops receiving requests on its
 *       service endpoint and replying to each caller through its reply object,
 *       dispatching by message label to the registered handlers.
 * @server: The server to run.
 */
void run(server_s* server);
