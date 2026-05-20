#pragma once

#include "../../include/uapi.h"

/**
 * sys_ipc_send - Sends a message to another task port.
 * @dst: The destination task id.
 * @msg: The message to send.
 * @return: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_send(u32 dst, const ipc_msg_t* msg);

/**
 * sys_ipc_recv - Receives one message from the caller task port.
 * @msg: The output message buffer.
 * @return: E_OK once a message has been copied into @msg.
 */
i32 sys_ipc_recv(ipc_msg_t* msg);

/**
 * sys_ipc_call - Sends a request and waits for a reply.
 * @dst: The destination task id.
 * @msg: The in-out message buffer; request on entry, reply on return.
 * @return: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_call(u32 dst, ipc_msg_t* msg);

/**
 * sys_ipc_reply - Delivers a reply to a client thread.
 * @client: The client thread id.
 * @msg: The reply message to deliver.
 * @return: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_reply(u32 client, const ipc_msg_t* msg);

/**
 * sys_thread_create - Creates a new thread in the current task.
 * @entry: The thread entry function.
 * @priority: The initial scheduler priority.
 * @return: The new thread id on success, or a negative error code on failure.
 */
i32 sys_thread_create(void (*entry)(void), u32 priority);

/**
 * sys_thread_yield - Voluntarily yields the CPU.
 * @return: E_OK on success, or a negative error code on failure.
 */
i32 sys_thread_yield(void);

/**
 * sys_thread_exit - Terminates the calling thread.
 * @code: The exit status passed to the kernel.
 */
void sys_thread_exit(i32 code) __attribute__((__noreturn__));

/**
 * sys_log_read - Reads bytes from the kernel log ring buffer.
 * @dst: The output buffer.
 * @len: The maximum number of bytes to read.
 * @off: The offset from the oldest available byte.
 * @return: The number of bytes copied on success, or a negative error code on
 *          failure.
 */
i32 sys_log_read(void* dst, u32 len, u32 off);

/**
 * sys_map_pg - Maps a physical page into the caller address space.
 * @vaddr: The user virtual address to map.
 * @paddr: The physical address of the page.
 * @flags: The mapping flags, such as VM_FLAG_RW and VM_FLAG_USER.
 * @return: E_OK on success, or a negative error code on failure.
 */
i32 sys_map_pg(void* vaddr, phys_addr_t paddr, u32 flags);

/**
 * sys_unmap_pg - Removes the mapping at @vaddr in the caller address space.
 *                No-op if the page is not currently mapped.
 * @vaddr: The user virtual address to unmap; must be page-aligned.
 * @return: E_OK on success, -E_INVAL if @vaddr is not page-aligned,
 *          -E_FAULT if @vaddr is zero or lies in kernel space.
 */
i32 sys_unmap_pg(void* vaddr);
