#pragma once

#include <config.h>
#include <kernel/capability/capability.h>
#include <kernel/sync/spinlock.h>
#include <libk/list.h>
#include <stdbool.h>
#include <uapi/ipc.h>
#include <uapi/types.h>

typedef enum endpoint_state_e {
    ENDPOINT_STATE_IDLE = 0, /* Empty queue      */
    ENDPOINT_STATE_SEND = 1, /* Senders queued   */
    ENDPOINT_STATE_RECV = 2  /* Receivers queued */
} endpoint_state_e;

typedef struct endpoint_s {
    spinlock_s       lock;  /* Protects the endpoint          */
    endpoint_state_e state; /* Endpoint queue state           */
    list_node_s      queue; /* Endpoint blocked senders/recvs */
} endpoint_s;

typedef struct notification_s {
    spinlock_s  lock;    /* Protects the notification  */
    u32         signals; /* Notification signal bits   */
    list_node_s waiters; /* Notification blocked recvs */
} notification_s;

typedef struct reply_s {
    struct thread_ctrl_blk_s* caller; /* Blocked client */
} reply_s;

/**
 * endpoint_init - Initializes @ep as an idle endpoint.
 * @ep: The endpoint to initialize.
 */
void endpoint_init(endpoint_s* ep);

/**
 * notification_init - Initializes @nt as an empty notification.
 * @nt: The notification to initialize.
 */
void notification_init(notification_s* nt);

/**
 * reply_init - Initializes @reply as an empty reply.
 * @reply: The reply to initialize.
 */
void reply_init(reply_s* reply);

/**
 * notification_signal - Sets @bits on @nt's signals, wakes up a blocked thread,
 *                       and resets the signals.
 * @nt: The notification to signal.
 * @bits: The signal bits to set.
 */
void notification_signal(notification_s* nt, u32 bits);

/**
 * ipc_send - Resolves @ep_cptr to an endpoint capability, sends @packet over it,
 *            and blocks until a receiver takes the message.
 * @root: The CSpace root capability.
 * @ep_cptr: The endpoint capability pointer.
 * @mi: The message tag.
 * @packet: The message buffer in the caller's address space.
 * @grant_cptr: The capability pointer to grant, if any.
 * @nonblock: Whether to perform a non-blocking send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_send(const capability_s* root,
             u32                 ep_cptr,
             msg_info_t          mi,
             ipc_packet_s*       packet,
             u32                 grant_cptr,
             bool                nonblock);

/**
 * ipc_call - Resolves @ep_cptr to an endpoint capability, sends @packet over it,
 *            and blocks until the receiver replies.
 * @root: The CSpace root capability to resolve from.
 * @ep_cptr: The endpoint capability pointer.
 * @mi: The message tag.
 * @packet: The in-out message buffer in the caller's address space.
 * @grant_cptr: The capability pointer to grant, if any.
 * Returns: The reply message tag on success, or a negative error code on failure.
 */
i32 ipc_call(const capability_s* root,
             u32                 ep_cptr,
             msg_info_t          mi,
             ipc_packet_s*       packet,
             u32                 grant_cptr);

/**
 * ipc_recv - Resolves @ep_cptr to an endpoint capability, receives a message
 *            into @packet, and blocks until a sender arrives.
 * @root: The CSpace root capability to resolve from.
 * @ep_cptr: The endpoint capability pointer.
 * @reply_cptr: The reply object capability pointer.
 * @packet: The receive buffer in the caller's address space.
 * @badge: Out-parameter for the sender's badge. Ignored if NULL.
 * @recv_slot: The empty slot accepting a granted capability, if any.
 * @nonblock: Whether to perform a non-blocking receive.
 * Returns: The received message tag on success, or a negative error code on failure.
 */
i32 ipc_recv(const capability_s* root,
             u32                 ep_cptr,
             u32                 reply_cptr,
             ipc_packet_s*       packet,
             u32*                badge,
             u32                 recv_slot,
             bool                nonblock);

/**
 * ipc_reply - Replies to the caller bound to @reply_cptr.
 * @root: The CSpace root capability to resolve from.
 * @reply_cptr: The reply object capability pointer.
 * @mi: The message tag.
 * @packet: The reply buffer in the replier's address space.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_reply(const capability_s* root, u32 reply_cptr, msg_info_t mi, ipc_packet_s* packet);

/**
 * ipc_replyrecv - Replies to the caller bound to @reply_cptr, then receives the
 *                 next message on @ep_cptr, binding it to @reply_cptr.
 * @root: The CSpace root capability to resolve from.
 * @ep_cptr: The endpoint capability pointer.
 * @reply_cptr: The reply object capability pointer.
 * @mi: The message tag.
 * @packet: The in-out message buffer in the server's address space.
 * @badge: Out-parameter for the next sender's badge. Ignored if NULL.
 * Returns: The next received message tag on success, or a negative error code.
 */
i32 ipc_replyrecv(const capability_s* root,
                  u32                 ep_cptr,
                  u32                 reply_cptr,
                  msg_info_t          mi,
                  ipc_packet_s*       packet,
                  u32*                badge);

/**
 * ipc_signal - Resolves @ntfn_cptr to a notification capability, signalling it with the 
 *              capability slot's badge.
 * @root: The CSpace root capability to resolve from.
 * @ntfn_cptr: The notification capability pointer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_signal(const capability_s* root, u32 ntfn_cptr);

/**
 * ipc_wait - Resolves @ntfn_cptr to a notification capability, waits on it, and blocks
 *            until at least one signal bit is set.
 * @root: The CSpace root capability to resolve from.
 * @ntfn_cptr: The notification capability pointer.
 * @badge: Out-parameter for the consumed signal bits. Ignored if NULL.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_wait(const capability_s* root, u32 ntfn_cptr, u32* badge);
