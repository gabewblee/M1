#pragma once

#include <uapi/uapi.h>

/**
 * sys_ipc_send - Sends @packet to task @dst's port. Blocks while the port is full,
 *                until space becomes available.
 * @dst: The destination task ID.
 * @packet: The packet to send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_send(u32 dst, ipc_packet_s* packet);

/**
 * sys_ipc_recv - Receives a packet into @packet from the caller's port. Blocks
 *                until a packet is available.
 * @packet: The output packet buffer.
 * Returns: E_OK on success.
 */
i32 sys_ipc_recv(ipc_packet_s* packet);

/**
 * sys_ipc_call - Sends @packet to task @dst's port, then blocks until a reply is
 *                received into @packet. Requires the caller to have a reply port.
 * @dst: The destination task ID.
 * @packet: The in-out packet buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_call(u32 dst, ipc_packet_s* packet);

/**
 * sys_ipc_reply - Sends @packet to thread @client's reply port, waking it if it is
 *                 blocked awaiting the reply.
 * @client: The destination thread ID.
 * @packet: The packet to send.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_ipc_reply(u32 client, ipc_packet_s* packet);

/**
 * sys_thread_create - Creates a new thread in the current task.
 * @entry: The thread's initial entry function.
 * @priority: The thread's initial scheduling priority.
 * Returns: The new thread ID on success, or a negative error code on failure.
 */
i32 sys_thread_create(void (*entry)(void), u32 priority);

/**
 * sys_thread_yield - Yields the CPU to the highest priority runnable thread.
 * Returns: E_OK on success.
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
 * Returns: E_OK on success.
 */
i32 sys_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * sys_unmap_pg - Unmaps the mapping at @vaddr. No-op if @vaddr is not mapped.
 * @vaddr: The virtual address to unmap.
 * Returns: E_OK on success.
 */
i32 sys_unmap_pg(void* vaddr);

/**
 * sys_log_read - Reads at most @len bytes from the kernel log ring buffer,
 *                starting at @off.
 * @dst: The output buffer.
 * @len: The maximum number of bytes to read.
 * @off: The offset from the oldest available byte.
 * Returns: The number of bytes copied to @dst.
 */
i32 sys_log_read(void* dst, u32 len, u32 off);

/**
 * sys_server_lookup - Looks up the task ID associated with @id.
 * @id: The server ID to lookup.
 * Returns: The task ID on success, or a negative error code on failure.
 */
i32 sys_server_lookup(u32 id);

/**
 * sys_irq_register_handler - Registers a handler for IRQ @irq. The handler
 *                            unblocks all threads waiting for IRQ @irq.
 * @irq: The IRQ number to register.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_irq_register_handler(u32 irq);

/**
 * sys_irq_wait_for - Waits for IRQ @irq to fire.
 * @irq: The IRQ number to wait for.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_irq_wait_for(u32 irq);

/**
 * sys_vm_copy - Transfers @len bytes between the caller's address space and
 *               thread @id's address space.
 * 
 *               @dir controls direction:
 *               - VM_COPY_FROM_PEER: copies from @dst in peer into @src in caller.
 *               - VM_COPY_TO_PEER: copies from @src in caller into @dst in peer.
 * @id: The peer thread ID.
 * @dst: Buffer in the peer's address space.
 * @src: Buffer in the caller's address space.
 * @len: Number of bytes to transfer.
 * @dir: VM_COPY_FROM_PEER or VM_COPY_TO_PEER.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 sys_vm_copy(u32 id, void* dst, void* src, u32 len, u32 dir);