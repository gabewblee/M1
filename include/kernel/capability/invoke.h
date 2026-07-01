#pragma once

#include <config.h>
#include <kernel/capability/capability.h>
#include <kernel/core/thread.h>
#include <uapi/capability.h>

/**
 * capability_invoke - Performs the @label method on the already-resolved
 *                     capability @slot, resolving any operand capability
 *                     pointers from @args against @cur's CSpace. Invoked from
 *                     the Call path when the target capability is not an
 *                     endpoint, mirroring seL4 object invocation.
 * @cur: The invoking thread.
 * @slot: The invoked capability's table entry.
 * @label: The method label, one of CAPABILITY_LABEL_*.
 * @args: The CAPABILITY_INVOKE_ARGC method arguments (the message registers).
 * @argc: The number of arguments supplied.
 * Returns: E_OK on success, or a negative error code on failure.
 *
 * Argument layout by label:
 * - UNTYPED_RETYPE:  [type, size_bits, dst_cnode_cptr, dst_cnode_depth, dst_index, count]
 * - CNODE_COPY:      [dst_cptr, dst_depth]
 * - CNODE_MINT:      [dst_cptr, dst_depth, rights, badge]
 * - CNODE_MOVE:      [dst_cptr, dst_depth]
 * - CNODE_DELETE:    []
 * - CNODE_REVOKE:    []
 * - TCB_CONFIGURE:   [cspace_cptr, cspace_depth, vspace_cptr, vspace_depth, entry, stack, prio, user]
 * - TCB_RESUME:      []
 * - PAGE_MAP:        [vspace_cptr, vspace_depth, vaddr, flags]
 * - PAGE_UNMAP:      [vspace_cptr, vspace_depth, vaddr]
 * - PAGE_TABLE_MAP:  [vspace_cptr, vspace_depth, vaddr]
 * - IRQ_GET:         [irq, dst_cnode_cptr, dst_cnode_depth, dst_index]
 * - IRQ_SET_NTFN:    [ntfn_cptr, ntfn_depth]
 */
i32 capability_invoke(thread_ctrl_blk_s* cur, cte_s* slot, u32 label, const u32* args, u32 argc);
