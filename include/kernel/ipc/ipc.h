#pragma once

#include <config.h>
#include <ipc.h>
#include <kernel/sync/spinlock.h>
#include <kernel/syscall.h>
#include <libk/list.h>
#include <uapi/ipc.h>

typedef struct task_port_s {
    spinlock_s  lock;    /* Protects all fields below      */
    list_node_s packets; /* Packet queue                   */
    list_node_s recvs;   /* Receivers blocked on the port  */
    list_node_s sndrs;   /* Senders blocked on the port    */
    u32         cnt;     /* Current packet count           */
    u32         cap;     /* Maximum packet count           */
} task_port_s;

typedef struct kipc_packet_s {
    ipc_packet_s packet; /* IPC packet         */
    list_node_s  link;   /* Packet queue node  */
} kipc_packet_s;

/**
 * port_create - Allocates a port with capacity @cap.
 * @cap: The maximum number of buffered packets.
 * Returns: The new port, or NULL on failure.
 */
task_port_s* port_create(u32 cap);

/**
 * port_destroy - Destroys @port, freeing any queued packets.
 * @port: The port to destroy.
 */
void port_destroy(task_port_s* port);

/**
 * ipc_send - Sends @packet to task @dst's port. Blocks on the port's sender queue
 *            if the port is full.
 * @dst: The destination task ID.
 * @packet: The packet to send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_send(u32 dst, ipc_packet_s* packet);

/**
 * ipc_recv - Receives a packet into @packet from the caller's port. Blocks on
 *            the port's receiver queue if the port is empty.
 * @packet: The output packet buffer.
 * Returns: E_OK on success.
 */
i32 ipc_recv(ipc_packet_s* packet);

/**
 * ipc_call - Sends @packet to task @dst's port, then blocks on the caller's reply
 *            port until a reply is received.
 * @packet: The in-out packet buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_call(u32 dst, ipc_packet_s* packet);

/**
 * ipc_reply - Sends @packet to thread @dst's reply port. Unblocks the sender if it
 *             is waiting for a reply.
 * @dst: The destination thread ID.
 * @packet: The packet to send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_reply(u32 dst, ipc_packet_s* packet);

/**
 * ipc_init - Initializes the inter-process communication (IPC) subsystem.
 */
void __init ipc_init(void);
