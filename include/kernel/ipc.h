#pragma once

#include "config.h"
#include "kernel/syscall.h"
#include "libk/list.h"
#include "uapi/ipc.h"

typedef struct port_s {
    list_node_s msgs;      /* Message queue                 */
    list_node_s receivers; /* Receivers blocked on the port */
    list_node_s senders;   /* Senders blocked on the port   */
    u32         cnt;       /* Current message count         */
    u32         cap;       /* Maximum message count         */
} port_s;

typedef struct ipc_msg_ext_s {
    ipc_msg_s   msg;  /* IPC message        */
    list_node_s link; /* Message queue node */
} ipc_msg_ext_s;

/**
 * port_create - Allocates a port with capacity @capacity.
 * @capacity: The maximum number of buffered messages.
 * Returns: The new port, or NULL on failure.
 */
port_s* port_create(u32 capacity);

/**
 * port_destroy - Destroys @port. Drops all queued messages, frees all
 *                queued messages, and frees @port.
 * @port: The port to destroy.
 */
void port_destroy(port_s* port);

/**
 * ipc_send - Sends @msg to task @dst's port. Does not block the caller.
 * @dst: The destination task ID.
 * @msg: The message to send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_send(u32 dst, const ipc_msg_s* msg);

/**
 * ipc_recv - Receives a message into @out from the caller's port. Works
 *            in conjunction with ipc_send().
 * @out: The output message buffer.
 * Returns: E_OK on success.
 */
i32 ipc_recv(ipc_msg_s* out);

/**
 * ipc_call - Atomic ipc_send() and ipc_recv() operation. Sends @msg to
 *            task @dst's port and waits for a reply in @msg. Blocks the
 *            caller.
 * @dst: The destination task ID.
 * @msg: The in-out message buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_call(u32 dst, ipc_msg_s* msg);

/**
 * ipc_reply - Delivers @msg to thread @client's reply port. Works in
 *             conjunction with ipc_call().
 * @client: The client thread ID.
 * @msg: The reply message to deliver.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_reply(u32 client, const ipc_msg_s* msg);

/**
 * ipc_init - Initializes IPC subsystem. Currently no-op.
 */
void __init ipc_init(void);