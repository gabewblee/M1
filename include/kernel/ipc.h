#pragma once

#include "config.h"
#include "lib/list.h"

typedef struct ipc_msg_t {
    u32         id;                  /* Message identifier          */
    u32         sender;              /* Originating thread id       */
    u32         sz;                  /* Valid payload size          */
    u8          data[IPC_INLINE_SZ]; /* Inline message payload data */
    list_node_t link;                /* Queue membership node       */
} ipc_msg_t;

typedef struct port_t {
    list_node_t messages;  /* Queue of buffered messages       */
    list_node_t receivers; /* Threads blocked waiting for data */
    list_node_t senders;   /* Threads blocked waiting for room */
    u32         cnt;       /* Current message count in queue   */
    u32         cap;       /* Maximum message count allowed    */
} port_t;

/**
 * port_create - Allocates a port with bounded buffered capacity.
 * @capacity: The maximum buffered messages, where 0 means rendezvous only.
 * @return: The new port, or NULL on failure.
 */
port_t* port_create(u32 capacity);

/**
 * port_destroy - Releases a port and drops queued messages. Caller 
 *                must guarantee no thread is blocked on the port
 *                wait queues.
 * @port: The port to destroy.
 */
void port_destroy(port_t* port);

/**
 * ipc_send - Sends a message to another task port. Blocks while
 *            destination queue is full when buffering is enabled.
 * @dst: The destination task id.
 * @msg: The message to send.
 * @return: E_OK on success, -E_INVAL for unknown task, or -E_NOMEM on
 *          allocation failure.
 */
i32 ipc_send(u32 dst, const ipc_msg_t* msg);

/**
 * ipc_recv - Receives one message from the caller task port. Blocks 
 *            until a message is available.
 * @out: The output message buffer.
 * @return: E_OK once a message has been copied into @out.
 */
i32 ipc_recv(ipc_msg_t* out);

/**
 * ipc_call - Sends a request and waits for a reply.
 * @dst: The destination task id.
 * @msg: The in out message buffer, request on entry and reply on return.
 * @return: E_OK on success, -E_INVAL for unknown task, -E_NOMEM on
 *          allocation failure, or -E_PERM on permission failure.
 */
i32 ipc_call(u32 dst, ipc_msg_t* msg);

/**
 * ipc_reply - Delivers a reply to a client thread private reply port.
 * @client: The client thread id.
 * @msg: The reply message to deliver.
 * @return: E_OK on success, -E_INVAL for unknown thread or missing reply
 *          port, or -E_NOMEM on allocation failure.
 */
i32 ipc_reply(u32 client, const ipc_msg_t* msg);

/**
 * ipc_init - Initializes IPC global state.
 */
void __init ipc_init(void);
