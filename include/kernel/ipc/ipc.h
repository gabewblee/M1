#pragma once

#include <config.h>
#include <ipc.h>
#include <kernel/sync/spinlock.h>
#include <kernel/syscall.h>
#include <libk/list.h>
#include <uapi/ipc.h>

typedef struct port_s {
    spinlock_s  lock;  /* Protects all fields below     */
    list_node_s msgs;  /* Message queue                 */
    list_node_s recvs; /* Receivers blocked on the port */
    list_node_s sndrs; /* Senders blocked on the port   */
    u32         cnt;   /* Current message count         */
    u32         cap;   /* Maximum message count         */
} port_s;

typedef struct kipc_msg_s {
    ipc_msg_s   msg;  /* IPC message        */
    list_node_s link; /* Message queue node */
} kipc_msg_s;

/**
 * port_create - Allocates a port with capacity @cap.
 * @cap: The maximum number of buffered messages.
 * Returns: The new port, or NULL on failure.
 */
port_s* port_create(u32 cap);

/**
 * port_destroy - Destroys @port, freeing any queued messages.
 * @port: The port to destroy.
 */
void port_destroy(port_s* port);

/**
 * ipc_send - Sends @msg to task @dst's port. Blocks on the port's sender queue
 *            if the port is full.
 * @dst: The destination task ID.
 * @msg: The message to send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_send(u32 dst, ipc_msg_s* msg);

/**
 * ipc_recv - Receives a message into @msg from the caller's port. Blocks on
 *            the port's receiver queue if the port is empty.
 * @msg: The output message buffer.
 * Returns: E_OK on success.
 */
i32 ipc_recv(ipc_msg_s* msg);

/**
 * ipc_call - Sends @msg to task @dst's port, then blocks on the caller's reply
 *            port until a reply is received.
 * @msg: The in-out message buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_call(u32 dst, ipc_msg_s* msg);

/**
 * ipc_reply - Sends @msg to thread @dst's reply port. Unblocks the sender if it
 *             is waiting for a reply.
 * @dst: The destination thread ID.
 * @msg: The message to send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_reply(u32 dst, ipc_msg_s* msg);

/**
 * ipc_init - Initializes the inter-process communication (IPC) subsystem.
 */
void __init ipc_init(void);
