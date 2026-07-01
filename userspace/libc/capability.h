#pragma once

#include <uapi/uapi.h>

/* Userspace bindings for seL4-style capability invocations. Each invocation is
   a Call on the invoked capability: the method label travels in the message
   tag and the operand words travel in the message registers. Operand
   capability pointers are resolved at the depth the caller supplies, against
   the calling thread's own CSpace. */

/**
 * capability_retype - Retypes @count objects of @type out of the untyped
 *                     capability at @untyped into consecutive slots of the
 *                     CNode at @dst_cnode (resolved at @dst_depth), starting at
 *                     slot @dst_index.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_retype(u32 untyped, u32 type, u32 size_bits,
                      u32 dst_cnode, u32 dst_depth, u32 dst_index, u32 count);

/* CNode operations are invocations on a CNode capability - the @service that
   roots the slots they address - exactly as in seL4. Routing them through the
   destination CNode rather than the operand capability keeps the Call target a
   CNode, so it is never mistaken for endpoint IPC even when the capability being
   moved is itself an endpoint. The destination and source slots are resolved
   against @service at @dst_depth and @src_depth respectively. */

/**
 * capability_copy - Copies the capability at (@service, @src_index, @src_depth)
 *                   into the empty slot (@service, @dst_index, @dst_depth).
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_copy(u32 service, u32 dst_index, u32 dst_depth, u32 src_index, u32 src_depth);

/**
 * capability_mint - Derives the capability at (@service, @src_index, @src_depth)
 *                   into the empty slot (@service, @dst_index, @dst_depth) with
 *                   rights masked by @rights and the given @badge.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_mint(u32 service, u32 dst_index, u32 dst_depth,
                    u32 src_index, u32 src_depth, u32 rights, u32 badge);

/**
 * capability_move - Moves the capability at (@service, @src_index, @src_depth)
 *                   into the empty slot (@service, @dst_index, @dst_depth).
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_move(u32 service, u32 dst_index, u32 dst_depth, u32 src_index, u32 src_depth);

/**
 * capability_delete - Deletes the capability at (@service, @index, @depth).
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_delete(u32 service, u32 index, u32 depth);

/**
 * capability_revoke - Revokes every capability derived from the capability at
 *                     (@service, @index, @depth).
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_revoke(u32 service, u32 index, u32 depth);

/**
 * tcb_configure - Configures the TCB at @tcb with the CSpace root @cspace and
 *                 VSpace @vspace (resolved at @cspace_depth / @vspace_depth),
 *                 initial @entry, user stack pointer @stack, and @priority.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 tcb_configure(u32 tcb, u32 cspace, u32 cspace_depth, u32 vspace, u32 vspace_depth,
                  u32 entry, u32 stack, u32 priority);

/**
 * tcb_resume - Makes the configured TCB at @tcb schedulable.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 tcb_resume(u32 tcb);

/**
 * page_table_map - Installs the page table at @table into the VSpace @vspace
 *                  (resolved at @vspace_depth) so it backs @vaddr.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 page_table_map(u32 table, u32 vspace, u32 vspace_depth, u32 vaddr);

/**
 * page_map - Maps the frame at @frame into the VSpace @vspace (resolved at
 *            @vspace_depth) at @vaddr with @flags.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 page_map(u32 frame, u32 vspace, u32 vspace_depth, u32 vaddr, u32 flags);

/**
 * page_unmap - Unmaps the frame at @frame from the VSpace @vspace (resolved at
 *              @vspace_depth) at @vaddr.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 page_unmap(u32 frame, u32 vspace, u32 vspace_depth, u32 vaddr);

/**
 * irq_control_get - Creates an IRQ handler capability for line @irq into the
 *                   empty slot @dst_index of the CNode at @dst_cnode (resolved
 *                   at @dst_depth), deriving it from the IRQ control capability
 *                   at @irq_control.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_control_get(u32 irq_control, u32 irq, u32 dst_cnode, u32 dst_depth, u32 dst_index);

/**
 * irq_handler_set_notification - Binds the notification at @ntfn (resolved at
 *                                @ntfn_depth) to the IRQ handler at @handler.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_handler_set_notification(u32 handler, u32 ntfn, u32 ntfn_depth);
