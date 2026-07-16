#pragma once

#include <uapi/uapi.h>

/**
 * sys_send - Resolves @cptr to an endpoint capability, sends @msg over it,
 *            and blocks until a receiver receives.
 * @cptr: The endpoint capability pointer.
 * @mi: The message tag.
 * @msg: The message buffer in the caller's address space.
 * @grant_cptr: The capability pointer to grant, if any.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_send(u32 cptr, msg_info_t mi, ipc_msg_s* msg, u32 grant_cptr);

/**
 * sys_nbsend - Resolves @cptr to an endpoint capability, sends @msg over it,
 *              and fails when no receiver is ready.
 * @cptr: The endpoint capability pointer.
 * @mi: The message tag.
 * @msg: The message buffer in the caller's address space.
 * @grant_cptr: The capability pointer to grant, if any.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_nbsend(u32 cptr, msg_info_t mi, ipc_msg_s* msg, u32 grant_cptr);

/**
 * sys_call - Resolves @cptr to a capability. If @cptr is an endpoint,
 *            sends @msg over it, and blocks until the receiver replies.
 *            Else, performs an object invocation based on @mi's label.
 * @cptr: The capability pointer.
 * @mi: The message tag.
 * @msg: The message buffer in the caller's address space.
 * @grant_cptr: The capability pointer to grant, if any.
 * Returns: The reply message tag on success, or a negative error code on failure.
 */
i32 sys_call(u32 cptr, msg_info_t mi, ipc_msg_s* msg, u32 grant_cptr);

/**
 * sys_recv - Resolves @cptr to an endpoint capability, receives a message
 *            into @msg, and blocks until a sender sends.
 * @cptr: The endpoint capability pointer.
 * @reply_cptr: The reply capability pointer.
 * @msg: The receive buffer in the caller's address space.
 * @badge: The out-parameter for the sender's badge.
 * @recv_slot: The empty slot accepting a granted capability, if any.
 * Returns: The received message tag on success, or a negative error code on failure.
 */
i32 sys_recv(u32 cptr, u32 reply_cptr, ipc_msg_s* msg, u32* badge, u32 recv_slot);

/**
 * sys_nbrecv - Resolves @cptr to an endpoint capability, receives a message
 *              into @msg, and fails when no sender is ready.
 * @cptr: The endpoint capability pointer.
 * @reply_cptr: The reply capability pointer.
 * @msg: The receive buffer in the caller's address space.
 * @badge: The out-parameter for the sender's badge.
 * @recv_slot: The empty slot accepting a granted capability, if any.
 * Returns: The received message tag on success, or a negative error code on failure.
 */
i32 sys_nbrecv(u32 cptr, u32 reply_cptr, ipc_msg_s* msg, u32* badge, u32 recv_slot);

/**
 * sys_reply - Resolves @reply_cptr to a reply capability, sends @msg over it,
 *             and unblocks the sender.
 * @reply_cptr: The reply capability pointer.
 * @mi: The message tag.
 * @msg: The reply buffer in the replier's address space.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_reply(u32 reply_cptr, msg_info_t mi, ipc_msg_s* msg);

/**
 * sys_replyrecv - Resolves @cptr to an endpoint capability, resolves @reply_cptr
 *                 to a reply capability, replies to the sender, and receives
 *                 over the endpoint.
 * @cptr: The endpoint capability pointer.
 * @reply_cptr: The reply capability pointer.
 * @mi: The message tag.
 * @msg: The message buffer in the server's address space.
 * @badge: The out-parameter for the next sender's badge.
 * Returns: The next received message tag on success, or a negative error code.
 */
i32 sys_replyrecv(u32 cptr, u32 reply_cptr, msg_info_t mi, ipc_msg_s* msg, u32* badge);

/**
 * sys_signal - Resolves @cptr to a notification capability, signalling it with
 *              the capability slot's badge.
 * @cptr: The notification capability pointer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_signal(u32 cptr);

/**
 * sys_wait - Resolves @cptr to a notification capability, waits on it, and blocks
 *            until at least one signal bit is set.
 * @cptr: The notification capability pointer.
 * @badge: The out-parameter for the consumed signal bits.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_wait(u32 cptr, u32* badge);

/**
 * sys_yield - Yields to the highest priority thread.
 * Returns: E_OK on success.
 */
i32 sys_yield(void);

/**
 * sys_thread_exit - Terminates the running thread.
 * @code: The exit status passed to the kernel.
 */
void sys_thread_exit(i32 code);

/**
 * sys_dbg_puts - Writes @buf to the kernel debug console, up to @len bytes.
 * @buf: The buffer to write.
 * @len: The buffer length.
 * Returns: The number of bytes written on success, or a negative error code on failure.
 */
i32 sys_dbg_puts(const char* buf, u32 len);
