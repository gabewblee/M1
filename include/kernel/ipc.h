#pragma once

#include "config.h"
#include "lib/list.h"
#include "kernel/syscall.h"
#include "uapi/ipc.h"

typedef struct port_t {
    list_node_t messages;  /* Message queue                 */
    list_node_t receivers; /* Receivers blocked on the port */
    list_node_t senders;   /* Senders blocked on the port   */
    u32         cnt;       /* Current message count         */
    u32         cap;       /* Maximum message count         */
} port_t;

typedef struct ipc_msg_ext_t {
    ipc_msg_t   msg;  /* IPC message        */
    list_node_t link; /* Message queue node */
} ipc_msg_ext_t;

/**
 * port_create - Allocates a port with capacity @capacity.
 * @capacity: The maximum number of buffered messages.
 * Returns: The new port, or NULL on failure.
 */
port_t* port_create(u32 capacity);

/**
 * port_destroy - Destroys @port. Drops all queued messages, frees all
 *                enqueued messages, and frees @port.
 * @port: The port to destroy.
 */
void port_destroy(port_t* port);

/**
 * ipc_send - Sends @msg to @dst.
 * @dst: The destination task ID.
 * @msg: The message to send.
 * Returns: E_OK on success, -E_INVAL on invalid @dst, or -E_NOMEM on
 *          allocation failure.
 */
i32 ipc_send(u32 dst, const ipc_msg_t* msg);

/**
 * ipc_recv - Receives a message into @out from the caller's port.
 * @out: The output message buffer.
 * Returns: E_OK on success.
 */
i32 ipc_recv(ipc_msg_t* out);

/**
 * ipc_call - Atomic ipc_send() and ipc_recv() operation. Sends @msg to
 *            @dst and waits for a reply in @msg.
 * @dst: The destination task ID.
 * @msg: The in-out message buffer.
 * Returns: E_OK on success, -E_INVAL on invalid @dst, -E_NOMEM on
 *          allocation failure, or -E_PERM on missing reply port.
 */
i32 ipc_call(u32 dst, ipc_msg_t* msg);

/**
 * ipc_reply - Delivers @msg to @client.
 * @client: The client thread ID.
 * @msg: The reply message to deliver.
 * Returns: E_OK on success, -E_INVAL on invalid @client or missing
 *          reply port, or -E_NOMEM on allocation failure.
 */
i32 ipc_reply(u32 client, const ipc_msg_t* msg);

/**
 * ipc_init - Initializes IPC subsystem. Currently no-op.
 */
void __init ipc_init(void);
