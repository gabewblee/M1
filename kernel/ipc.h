#pragma once

#include "../config.h"
#include "../lib/list.h"

#define IPC_MSG_INLINE   56
#define IPC_DEF_CAPACITY 16

typedef struct ipc_msg_t {
    list_node_t link;
    u32         sender;
    u32         id;
    u32         size;
    u8          data[IPC_MSG_INLINE];
} ipc_msg_t;

typedef struct port_t {
    list_node_t messages;
    list_node_t receivers;
    list_node_t senders;
    u32         queue_len;
    u32         queue_cap;
} port_t;

/**
 * port_create - Allocates a port with the given queue capacity. A capacity
 *               of 0 yields a purely synchronous rendezvous port.
 * @capacity: The capacity of the port.
 * @return: The new port, or NULL on failure.
 */
port_t* port_create(u32 capacity);

/**
 * port_destroy - Releases @port, dropping any queued messages. The caller must
 *                guarantee no thread is parked on @port->senders or 
 *                @port->receivers.
 * @port: The port to destroy.
 */
void port_destroy(port_t* port);

/**
 * ipc_send - Asynchronously delivers @msg to the task port of task @dst.
 *            Blocks the caller while the port queue is full.
 * @dst: The id of the destination task.
 * @msg: The message to send.
 * @return: E_OK on success, -E_INVAL for an unknown task, or -E_NOMEM on
 *          allocator failure.
 */
i32 ipc_send(u32 dst, const ipc_msg_t* msg);

/**
 * ipc_recv - Pulls one message off the caller's task port, blocking until
 *            one arrives. The sender field of @out is filled with the
 *            originating thread id.
 * @out: The pointer to the output message.
 * @return: E_OK.
 */
i32 ipc_recv(ipc_msg_t* out);

/**
 * ipc_call - Send @msg to task @dst, then block on the caller's 
 *            per-thread reply port. @msg is overwritten with the reply
 *            before returning.
 * @dst: The id of the destination task.
 * @msg: The message to send.
 * @return: E_OK on success, -E_INVAL for an unknown task, -E_NOMEM on
 *          allocation failure, or -E_PERM on permission failure.
 */
i32 ipc_call(u32 dst, ipc_msg_t* msg);

/**
 * ipc_reply - Delivers @msg to the private reply port of thread @client.
 * @client: The id of the client thread.
 * @msg: The message to deliver.
 * @return: E_OK on success, -E_INVAL for an unknown thread, or -E_NOMEM on
 *          allocation failure.
 */
i32 ipc_reply(u32 client, const ipc_msg_t* msg);

/**
 * ipc_init - Initializes the IPC subsystem. Currently a no-op.
 */
 void __init ipc_init(void);
