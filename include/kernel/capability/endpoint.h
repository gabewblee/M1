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

typedef struct ntfn_s {
    spinlock_s  lock;    /* Protects the notification  */
    u32         signals; /* Notification signal bits   */
    list_node_s waiters; /* Notification blocked recvs */
} ntfn_s;

typedef struct reply_s {
    struct thread_ctrl_blk_s* caller; /* Blocked client */
} reply_s;

/**
 * endpoint_init - Initializes @ep.
 * @ep: The endpoint to initialize.
 */
void endpoint_init(endpoint_s* ep);

/**
 * ntfn_init - Initializes @nt.
 * @nt: The notification to initialize.
 */
void ntfn_init(ntfn_s* nt);

/**
 * reply_init - Initializes @reply.
 * @reply: The reply to initialize.
 */
void reply_init(reply_s* reply);

/**
 * ntfn_signal - Sets @bits on @nt's signals, wakes up a blocked thread,
 *               and resets the signals.
 * @nt: The notification to signal.
 * @bits: The bits to set.
 */
void ntfn_signal(ntfn_s* nt, u32 bits);

/**
 * ipc_send - Resolves @ep_cptr to an endpoint capability, sends @msg over it,
 *            and blocks until a receiver receives.
 * @root: The CSpace root capability to resolve from.
 * @ep_cptr: The endpoint capability pointer.
 * @mi: The message tag.
 * @msg: The message buffer in the caller's address space.
 * @grant_cptr: The capability pointer to grant, if any.
 * @nonblock: Whether to perform a non-blocking send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_send(const capability_s* root,
             u32                 ep_cptr,
             msg_info_t          mi,
             ipc_msg_s*          msg,
             u32                 grant_cptr,
             bool                nonblock);

/**
 * ipc_call - Resolves @ep_cptr to an endpoint capability, sends @msg over it,
 *            and blocks until the receiver replies.
 * @root: The CSpace root capability to resolve from.
 * @ep_cptr: The endpoint capability pointer.
 * @mi: The message tag.
 * @msg: The message buffer in the caller's address space.
 * @grant_cptr: The capability pointer to grant, if any.
 * Returns: The reply message tag on success, or a negative error code on failure.
 */
i32 ipc_call(const capability_s* root,
             u32                 ep_cptr,
             msg_info_t          mi,
             ipc_msg_s*          msg,
             u32                 grant_cptr);

/**
 * ipc_recv - Resolves @ep_cptr to an endpoint capability, receives a message
 *            into @msg, and blocks until a sender sends.
 * @root: The CSpace root capability to resolve from.
 * @ep_cptr: The endpoint capability pointer.
 * @reply_cptr: The reply capability pointer.
 * @msg: The receive buffer in the caller's address space.
 * @badge: The sender's badge.
 * @recv_slot: The empty slot accepting a granted capability, if any.
 * @nonblock: Whether to perform a non-blocking receive.
 * Returns: The received message tag on success, or a negative error code on failure.
 */
i32 ipc_recv(const capability_s* root,
             u32                 ep_cptr,
             u32                 reply_cptr,
             ipc_msg_s*          msg,
             u32*                badge,
             u32                 recv_slot,
             bool                nonblock);

/**
 * ipc_reply - Resolves @reply_cptr to a reply capability, sends @msg, and unblocks
 *             the sender.
 * @root: The CSpace root capability to resolve from.
 * @reply_cptr: The reply capability pointer.
 * @mi: The message tag.
 * @msg: The reply buffer in the replier's address space.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_reply(const capability_s* root, u32 reply_cptr, msg_info_t mi, ipc_msg_s* msg);

/**
 * ipc_replyrecv - Resolves @ep_cptr to an endpoint capability, @reply_cptr to a reply
 *                 capability, replies to the sender, and receives over the endpoint.
 * @root: The CSpace root capability to resolve from.
 * @ep_cptr: The endpoint capability pointer.
 * @reply_cptr: The reply capability pointer.
 * @mi: The message tag.
 * @msg: The message buffer in the server's address space.
 * @badge: The next sender's badge.
 * Returns: The next received message tag on success, or a negative error code.
 */
i32 ipc_replyrecv(const capability_s* root,
                  u32                 ep_cptr,
                  u32                 reply_cptr,
                  msg_info_t          mi,
                  ipc_msg_s*          msg,
                  u32*                badge);

/**
 * ipc_signal - Resolves @ntfn_cptr to a notification capability, signaling it with the
 *              slot's badge.
 * @root: The CSpace root capability to resolve from.
 * @ntfn_cptr: The notification capability pointer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_signal(const capability_s* root, u32 ntfn_cptr);

/**
 * ipc_wait - Resolves @ntfn_cptr to a notification capability, blocking on it until a
 *            signal is received.
 * @root: The CSpace root capability to resolve from.
 * @ntfn_cptr: The notification capability pointer.
 * @badge: The consumed signal bits. Ignored if NULL.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ipc_wait(const capability_s* root, u32 ntfn_cptr, u32* badge);
