#pragma once

#include <uapi/uapi.h>

/**
 * capability_retype - Retypes @cnt objects from @untyped to @type in the destination slots
 *                     rooted at @dst_cnode.
 * @untyped: The untyped capability pointer.
 * @type: The object type to retype to.
 * @nbits: log2(object size) in bits.
 * @dst_cnode: The destination CNode capability pointer.
 * @dst_depth: The number of bits of @dst_cnode to resolve.
 * @dst_index: The first destination slot index.
 * @cnt: The number of objects to retype.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_retype(u32 untyped,
                      u32 type,
                      u32 nbits,
                      u32 dst_cnode,
                      u32 dst_depth,
                      u32 dst_index,
                      u32 cnt);

/**
 * capability_cp - Copies the capability at (@service, @src_index, @src_depth) into the empty
 *                 slot (@service, @dst_index, @dst_depth) with equal rights.
 * @service: The CNode capability pointer invoked for the Copy operation.
 * @dst_index: The destination slot index.
 * @dst_depth: The number of bits of @dst_index to resolve.
 * @src_index: The source slot index.
 * @src_depth: The number of bits of @src_index to resolve.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_cp(u32 service, u32 dst_index, u32 dst_depth, u32 src_index, u32 src_depth);

/**
 * capability_mint - Derives a child of the capability at (@service, @src_index, @src_depth)
 *                   into the empty slot (@service, @dst_index, @dst_depth) with diminished
 *                   @rights.
 * @service: The CNode capability pointer invoked for the Mint operation.
 * @dst_index: The destination slot index.
 * @dst_depth: The number of bits of @dst_index to resolve.
 * @src_index: The source slot index.
 * @src_depth: The number of bits of @src_index to resolve.
 * @rights: The rights to retain.
 * @badge: The endpoint badge.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_mint(u32 service,
                    u32 dst_index,
                    u32 dst_depth,
                    u32 src_index,
                    u32 src_depth,
                    u32 rights,
                    u32 badge);

/**
 * capability_mv - Moves the capability at (@service, @src_index, @src_depth) into the empty
 *                 slot (@service, @dst_index, @dst_depth), preserving rights.
 * @service: The CNode capability pointer invoked for the Move operation.
 * @dst_index: The destination slot index.
 * @dst_depth: The number of bits of @dst_index to resolve.
 * @src_index: The source slot index.
 * @src_depth: The number of bits of @src_index to resolve.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_mv(u32 service, u32 dst_index, u32 dst_depth, u32 src_index, u32 src_depth);

/**
 * capability_del - Deletes every capability derived from the capability at (@service, @index,
 *                  @depth), including the capability itself.
 * @service: The CNode capability pointer invoked for the Delete operation.
 * @index: The target slot index.
 * @depth: The number of bits of @index to resolve.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_del(u32 service, u32 index, u32 depth);

/**
 * capability_rvk - Deletes every capability derived from the capability at (@service, @index,
 *                  @depth), excluding the capability itself.
 * @service: The CNode capability pointer invoked for the Revoke operation.
 * @index: The target slot index.
 * @depth: The number of bits of @index to resolve.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_rvk(u32 service, u32 index, u32 depth);

/**
 * tcb_configure - Configures the retyped TCB at @tcb, allocating its kernel stack and thread
 *                 id and binding it to a user-mode context.
 * @tcb: The TCB capability pointer to configure.
 * @cspace: The CSpace root capability pointer.
 * @cspace_depth: The number of bits of @cspace to resolve.
 * @vspace: The VSpace capability pointer.
 * @vspace_depth: The number of bits of @vspace to resolve.
 * @entry: The thread's initial entry point.
 * @stack: The thread's initial user-mode stack pointer.
 * @priority: The thread's scheduling priority.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 tcb_configure(u32 tcb,
                  u32 cspace,
                  u32 cspace_depth,
                  u32 vspace,
                  u32 vspace_depth,
                  u32 entry,
                  u32 stack,
                  u32 priority);

/**
 * tcb_resume - Makes the configured TCB at @tcb schedulable.
 * @tcb: The TCB capability pointer to resume.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 tcb_resume(u32 tcb);

/**
 * map_pg - Maps the frame at @frame into the VSpace @vspace (resolved at
 *          @vspace_depth) at @vaddr with @flags.
 * @frame: The frame capability pointer.
 * @vspace: The VSpace capability pointer.
 * @vspace_depth: The number of bits of @vspace to resolve.
 * @vaddr: The virtual address to map.
 * @flags: The page flags.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 map_pg(u32 frame, u32 vspace, u32 vspace_depth, u32 vaddr, u32 flags);

/**
 * map_pg_table - Maps the page table at @table into the page directory @vspace (resolved at
 *                @vspace_depth) so it backs @vaddr.
 * @table: The page table capability pointer.
 * @vspace: The VSpace capability pointer.
 * @vspace_depth: The number of bits of @vspace to resolve.
 * @vaddr: The virtual address to map.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 map_pg_table(u32 table, u32 vspace, u32 vspace_depth, u32 vaddr);

/**
 * unmap_pg - Unmaps the mapping at @vaddr in the VSpace @vspace (resolved at @vspace_depth).
 *            No-op if @vaddr is not mapped.
 * @frame: The frame capability pointer.
 * @vspace: The VSpace capability pointer.
 * @vspace_depth: The number of bits of @vspace to resolve.
 * @vaddr: The virtual address to unmap.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 unmap_pg(u32 frame, u32 vspace, u32 vspace_depth, u32 vaddr);

/**
 * irq_ctrl_get - Creates an IRQ handler capability for line @irq into the empty slot @dst_index
 *                of the CNode at @dst_cnode (resolved at @dst_depth), deriving it from the IRQ
 *                control capability at @irq_ctrl.
 * @irq_ctrl: The IRQ control capability pointer.
 * @irq: The interrupt number.
 * @dst_cnode: The destination CNode capability pointer.
 * @dst_depth: The number of bits of @dst_cnode to resolve.
 * @dst_index: The destination slot index.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_ctrl_get(u32 irq_ctrl, u32 irq, u32 dst_cnode, u32 dst_depth, u32 dst_index);

/**
 * irq_handler_set_ntfn - Binds the notification at @ntfn (resolved at @ntfn_depth) to the IRQ
 *                        handler at @handler. Each time the handler's interrupt fires, @ntfn is
 *                        signalled with the interrupt bit set.
 * @handler: The IRQ handler capability pointer.
 * @ntfn: The notification capability pointer.
 * @ntfn_depth: The number of bits of @ntfn to resolve.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_handler_set_ntfn(u32 handler, u32 ntfn, u32 ntfn_depth);
