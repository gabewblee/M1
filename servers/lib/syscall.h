#pragma once

#include "uapi.h"

/**
 * sys_ipc_send - Sends @msg to @dst.
 * @dst: The destination task ID.
 * @msg: The message to send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_send(u32 dst, const ipc_msg_t* msg);

/**
 * sys_ipc_recv - Receives a message into @out from the caller's port.
 * @msg: The output message buffer.
 * Returns: E_OK on success.
 */
i32 sys_ipc_recv(ipc_msg_t* msg);

/**
 * sys_ipc_call - Atomic ipc_send() and ipc_recv() operation. Sends @msg to
 *                @dst and waits for a reply in @msg.
 * @dst: The destination task ID.
 * @msg: The in-out message buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_call(u32 dst, ipc_msg_t* msg);

/**
 * sys_ipc_reply - Delivers @msg to @client.
 * @client: The client thread ID.
 * @msg: The reply message to deliver.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_reply(u32 client, const ipc_msg_t* msg);

/**
 * sys_thread_create - Creates a new thread in the current task.
 * @entry: The thread's entry function.
 * @priority: The thread's initial scheduler priority.
 * Returns: The new thread ID on success, or a negative error code on failure.
 */
i32 sys_thread_create(void (*entry)(void), u32 priority);

/**
 * sys_thread_yield - Yields the CPU to the highest priority runnable thread.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_thread_yield(void);

/**
 * sys_thread_exit - Terminates the running thread.
 * @code: The exit status passed to the kernel.
 */
void sys_thread_exit(i32 code);

/**
 * sys_map_pg - Maps @vaddr to @paddr with @flags.
 * @paddr: The physical address to map to.
 * @vaddr: The virtual address to map.
 * @flags: The flags for the page table entry.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * sys_unmap_pg - Unmaps the mapping at @vaddr. No-op if @vaddr is not mapped.
 * @vaddr: The virtual address to unmap.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_unmap_pg(const void* vaddr);

/**
 * sys_log_read - Reads at most @len bytes from the kernel log ring buffer,
 *                starting at @off.
 * @dst: The output buffer.
 * @len: The maximum number of bytes to read.
 * @off: The offset from the oldest available byte.
 * Returns: The number of bytes copied to @dst on success, or a negative error
 *          code on failure.
 */
i32 sys_log_read(void* dst, u32 len, u32 off);

/**
 * sys_server_lookup - Retrieves the task ID associated with @name.
 * @name: The NULL-terminated server name.
 * Returns: The task ID on success, or a negative error code on failure.
 */
i32 sys_server_lookup(const char* name);
