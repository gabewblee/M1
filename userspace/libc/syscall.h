#pragma once

#include <uapi/uapi.h>

/* Userspace bindings for the seL4-faithful syscall set. Synchronous IPC carries
   a msg_info tag in a register alongside the message buffer; the kernel resolves
   every capability pointer against the calling thread's CSpace at its natural
   depth, so no depth argument is needed. */

/**
 * sys_send - Sends @packet over the endpoint capability at @cptr, blocking until
 *            a receiver takes the message.
 * @cptr: The capability pointer to the endpoint.
 * @mi: The message tag (label, capability count, length).
 * @packet: The message buffer.
 * @grant_cptr: The capability pointer to grant, or 0 to transfer none.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_send(u32 cptr, msg_info_t mi, ipc_packet_s* packet, u32 grant_cptr);

/**
 * sys_nbsend - Polling send: like sys_send but fails when no receiver is ready.
 * @cptr: The capability pointer to the endpoint.
 * @mi: The message tag.
 * @packet: The message buffer.
 * @grant_cptr: The capability pointer to grant, or 0 to transfer none.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_nbsend(u32 cptr, msg_info_t mi, ipc_packet_s* packet, u32 grant_cptr);

/**
 * sys_call - Sends @packet over the endpoint at @cptr and blocks for a reply.
 * @cptr: The capability pointer to the endpoint.
 * @mi: The request message tag.
 * @packet: The in-out message buffer.
 * @grant_cptr: The capability pointer to grant, or 0 to transfer none.
 * Returns: The reply message tag on success, or a negative error code.
 */
i32 sys_call(u32 cptr, msg_info_t mi, ipc_packet_s* packet, u32 grant_cptr);

/**
 * sys_recv - Receives a message over the endpoint at @cptr, blocking until a
 *            sender arrives.
 * @cptr: The capability pointer to the endpoint.
 * @reply_cptr: The reply object bound to a calling sender, or 0.
 * @packet: The receive buffer.
 * @badge: Out-parameter for the sender's badge, or NULL.
 * @recv_slot: The empty slot accepting a granted capability, or 0 to accept none.
 * Returns: The received message tag on success, or a negative error code.
 */
i32 sys_recv(u32 cptr, u32 reply_cptr, ipc_packet_s* packet, u32* badge, u32 recv_slot);

/**
 * sys_nbrecv - Polling receive: like sys_recv but fails when no sender is ready.
 * @cptr: The capability pointer to the endpoint.
 * @reply_cptr: The reply object bound to a calling sender, or 0.
 * @packet: The receive buffer.
 * @badge: Out-parameter for the sender's badge, or NULL.
 * @recv_slot: The empty slot accepting a granted capability, or 0 to accept none.
 * Returns: The received message tag on success, or a negative error code.
 */
i32 sys_nbrecv(u32 cptr, u32 reply_cptr, ipc_packet_s* packet, u32* badge, u32 recv_slot);

/**
 * sys_reply - Replies to the caller bound to the reply object at @reply_cptr.
 * @reply_cptr: The capability pointer to the reply object.
 * @mi: The reply message tag.
 * @packet: The reply buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_reply(u32 reply_cptr, msg_info_t mi, ipc_packet_s* packet);

/**
 * sys_replyrecv - Replies to the caller bound to @reply_cptr, then receives the
 *                 next request over @cptr and binds it to @reply_cptr.
 * @cptr: The capability pointer to the endpoint.
 * @reply_cptr: The capability pointer to the reply object.
 * @mi: The reply message tag.
 * @packet: The in-out message buffer.
 * @badge: Out-parameter for the next sender's badge, or NULL.
 * Returns: The next received message tag on success, or a negative error code.
 */
i32 sys_replyrecv(u32 cptr, u32 reply_cptr, msg_info_t mi, ipc_packet_s* packet, u32* badge);

/**
 * sys_signal - Signals the notification capability at @cptr.
 * @cptr: The capability pointer to the notification.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_signal(u32 cptr);

/**
 * sys_wait - Waits on the notification capability at @cptr.
 * @cptr: The capability pointer to the notification.
 * @badge: Out-parameter for the consumed signal bits, or NULL.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_wait(u32 cptr, u32* badge);

/**
 * sys_yield - Yields the CPU to the highest priority runnable thread.
 * Returns: E_OK on success.
 */
i32 sys_yield(void);

/**
 * sys_thread_exit - Terminates the running thread.
 * @code: The exit status passed to the kernel.
 */
void sys_thread_exit(i32 code);

/**
 * sys_dbg_puts - Writes @len bytes of @str to the kernel debug console.
 * @str: The string to write.
 * @len: The number of bytes to write.
 * Returns: The number of bytes written, or a negative error code on failure.
 */
i32 sys_dbg_puts(const char* str, u32 len);
