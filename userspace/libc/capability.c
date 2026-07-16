#include <uapi/capability.h>
#include <userspace/libc/capability.h>
#include <userspace/libc/syscall.h>

static i32 invoke(u32 cptr, u32 label, const u32* args, u32 n) {
    ipc_msg_s msg;
    u32* regs = (u32*)msg.payload;
    for (u32 i = 0; i < CAPABILITY_INVOKE_ARGC; i++)
        regs[i] = (i < n) ? args[i] : 0u;

    return sys_call(cptr, mk_msg_info(label, 0, 0), &msg, 0);
}

i32 capability_retype(u32 untyped,
                      u32 type,
                      u32 nbits,
                      u32 dst_cnode,
                      u32 dst_depth,
                      u32 dst_idx,
                      u32 cnt) {
    u32 args[] = {type, nbits, dst_cnode, dst_depth, dst_idx, cnt};
    return invoke(untyped, CAPABILITY_LABEL_UNTYPED_RETYPE, args, 6);
}

i32 capability_cp(u32 service, u32 dst_idx, u32 dst_depth, u32 src_index, u32 src_depth) {
    u32 args[] = {dst_idx, dst_depth, src_index, src_depth};
    return invoke(service, CAPABILITY_LABEL_CNODE_CP, args, 4);
}

i32 capability_mint(u32 service,
                    u32 dst_idx,
                    u32 dst_depth,
                    u32 src_idx,
                    u32 src_depth,
                    u32 rights,
                    u32 badge) {
    u32 args[] = {dst_idx, dst_depth, src_idx, src_depth, rights, badge};
    return invoke(service, CAPABILITY_LABEL_CNODE_MINT, args, 6);
}

i32 capability_mv(u32 service, u32 dst_idx, u32 dst_depth, u32 src_idx, u32 src_depth) {
    u32 args[] = {dst_idx, dst_depth, src_idx, src_depth};
    return invoke(service, CAPABILITY_LABEL_CNODE_MV, args, 4);
}

i32 capability_del(u32 service, u32 idx, u32 depth) {
    u32 args[] = {idx, depth};
    return invoke(service, CAPABILITY_LABEL_CNODE_DEL, args, 2);
}

i32 capability_rvk(u32 service, u32 idx, u32 depth) {
    u32 args[] = {idx, depth};
    return invoke(service, CAPABILITY_LABEL_CNODE_RVK, args, 2);
}

i32 tcb_configure(u32 tcb, 
                  u32 cspace,
                  u32 cspace_depth,
                  u32 vspace,
                  u32 vspace_depth,
                  u32 entry,
                  u32 stack,
                  u32 priority) {
    u32 args[] = {cspace, cspace_depth, vspace, vspace_depth, entry, stack, priority, 1u};
    return invoke(tcb, CAPABILITY_LABEL_TCB_CONFIGURE, args, 8);
}

i32 tcb_resume(u32 tcb) {
    return invoke(tcb, CAPABILITY_LABEL_TCB_RESUME, 0, 0);
}

i32 map_pg(u32 frame, u32 vspace, u32 vspace_depth, u32 vaddr, u32 flags) {
    u32 args[] = {vspace, vspace_depth, vaddr, flags};
    return invoke(frame, CAPABILITY_LABEL_MAP_PG, args, 4);
}

i32 map_pg_table(u32 table, u32 vspace, u32 vspace_depth, u32 vaddr) {
    u32 args[] = {vspace, vspace_depth, vaddr};
    return invoke(table, CAPABILITY_LABEL_MAP_PG_TABLE, args, 3);
}

i32 unmap_pg(u32 frame, u32 vspace, u32 vspace_depth, u32 vaddr) {
    u32 args[] = {vspace, vspace_depth, vaddr};
    return invoke(frame, CAPABILITY_LABEL_UNMAP_PG, args, 3);
}

i32 irq_ctrl_get(u32 irq_ctrl, u32 irq, u32 dst_cnode, u32 dst_depth, u32 dst_idx) {
    u32 args[] = {irq, dst_cnode, dst_depth, dst_idx};
    return invoke(irq_ctrl, CAPABILITY_LABEL_IRQ_GET, args, 4);
}

i32 irq_handler_set_ntfn(u32 handler, u32 ntfn, u32 ntfn_depth) {
    u32 args[] = {ntfn, ntfn_depth};
    return invoke(handler, CAPABILITY_LABEL_IRQ_SET_NTFN, args, 2);
}
