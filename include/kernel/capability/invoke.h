#pragma once

#include <config.h>
#include <kernel/capability/capability.h>
#include <kernel/core/thread.h>
#include <uapi/capability.h>

/**
 * capability_invoke - Executes the @label method on capability @slot with the arguments @args.
 * @cur: The invoking thread.
 * @slot: The invoked capability's table entry.
 * @label: The method label, one of CAPABILITY_LABEL_*.
 * @args: The method arguments.
 * @argc: The method argument count.
 * Returns: E_OK on success, or a negative error code on failure.
 *
 * Arguments by label:
 * - UNTYPED_RETYPE: [type, nbits, cnode_cptr, cnode_depth, dst_index, cnt]
 * - CNODE_CP:       [dst_cptr, dst_depth, src_cptr, src_depth]
 * - CNODE_MINT:     [dst_cptr, dst_depth, src_cptr, src_depth, rights, badge]
 * - CNODE_MV:       [dst_cptr, dst_depth, src_cptr, src_depth]
 * - CNODE_DEL:      [cptr, depth]
 * - CNODE_RVK:      [cptr, depth]
 * - TCB_CONFIGURE:  [cspace_cptr, cspace_depth, vspace_cptr, vspace_depth, entry, stack, prio, user]
 * - TCB_RESUME:     []
 * - IRQ_GET:        [irq, dst_cnode_cptr, dst_cnode_depth, dst_index]
 * - IRQ_SET_NTFN:   [ntfn_cptr, ntfn_depth]
 * - MAP_PG:         [vspace_cptr, vspace_depth, vaddr, flags]
 * - MAP_PG_TABLE:   [vspace_cptr, vspace_depth, vaddr]
 * - UNMAP_PG:       [vspace_cptr, vspace_depth, vaddr]
 */
i32 capability_invoke(thread_ctrl_blk_s* cur, cte_s* slot, u32 label, const u32* args, u32 argc);
