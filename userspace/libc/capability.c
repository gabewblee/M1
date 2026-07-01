#include <uapi/capability.h>
#include <userspace/libc/capability.h>
#include <userspace/libc/syscall.h>

/* Marshals @n operand words into the message registers and Calls @cptr with
   method @label. The kernel resolves @cptr; when it is not an endpoint the
   Call is decoded as an object invocation. */
static i32 invoke(u32 cptr, u32 label, const u32* args, u32 n) {
    ipc_packet_s packet;
    u32*         regs = (u32*)packet.payload;

    for (u32 i = 0; i < CAPABILITY_INVOKE_ARGC; i++)
        regs[i] = (i < n) ? args[i] : 0u;

    return sys_call(cptr, mk_msg_info(label, 0, 0), &packet, 0);
}

i32 capability_retype(u32 untyped, u32 type, u32 size_bits,
                      u32 dst_cnode, u32 dst_depth, u32 dst_index, u32 count) {
    u32 args[] = {type, size_bits, dst_cnode, dst_depth, dst_index, count};
    return invoke(untyped, CAPABILITY_LABEL_UNTYPED_RETYPE, args, 6);
}

i32 capability_copy(u32 service, u32 dst_index, u32 dst_depth, u32 src_index, u32 src_depth) {
    u32 args[] = {dst_index, dst_depth, src_index, src_depth};
    return invoke(service, CAPABILITY_LABEL_CNODE_COPY, args, 4);
}

i32 capability_mint(u32 service, u32 dst_index, u32 dst_depth,
                    u32 src_index, u32 src_depth, u32 rights, u32 badge) {
    u32 args[] = {dst_index, dst_depth, src_index, src_depth, rights, badge};
    return invoke(service, CAPABILITY_LABEL_CNODE_MINT, args, 6);
}

i32 capability_move(u32 service, u32 dst_index, u32 dst_depth, u32 src_index, u32 src_depth) {
    u32 args[] = {dst_index, dst_depth, src_index, src_depth};
    return invoke(service, CAPABILITY_LABEL_CNODE_MOVE, args, 4);
}

i32 capability_delete(u32 service, u32 index, u32 depth) {
    u32 args[] = {index, depth};
    return invoke(service, CAPABILITY_LABEL_CNODE_DELETE, args, 2);
}

i32 capability_revoke(u32 service, u32 index, u32 depth) {
    u32 args[] = {index, depth};
    return invoke(service, CAPABILITY_LABEL_CNODE_REVOKE, args, 2);
}

i32 tcb_configure(u32 tcb, u32 cspace, u32 cspace_depth, u32 vspace, u32 vspace_depth,
                  u32 entry, u32 stack, u32 priority) {
    u32 args[] = {cspace, cspace_depth, vspace, vspace_depth, entry, stack, priority, 1u};
    return invoke(tcb, CAPABILITY_LABEL_TCB_CONFIGURE, args, 8);
}

i32 tcb_resume(u32 tcb) {
    return invoke(tcb, CAPABILITY_LABEL_TCB_RESUME, 0, 0);
}

i32 page_table_map(u32 table, u32 vspace, u32 vspace_depth, u32 vaddr) {
    u32 args[] = {vspace, vspace_depth, vaddr};
    return invoke(table, CAPABILITY_LABEL_PAGE_TABLE_MAP, args, 3);
}

i32 page_map(u32 frame, u32 vspace, u32 vspace_depth, u32 vaddr, u32 flags) {
    u32 args[] = {vspace, vspace_depth, vaddr, flags};
    return invoke(frame, CAPABILITY_LABEL_PAGE_MAP, args, 4);
}

i32 page_unmap(u32 frame, u32 vspace, u32 vspace_depth, u32 vaddr) {
    u32 args[] = {vspace, vspace_depth, vaddr};
    return invoke(frame, CAPABILITY_LABEL_PAGE_UNMAP, args, 3);
}

i32 irq_control_get(u32 irq_control, u32 irq, u32 dst_cnode, u32 dst_depth, u32 dst_index) {
    u32 args[] = {irq, dst_cnode, dst_depth, dst_index};
    return invoke(irq_control, CAPABILITY_LABEL_IRQ_GET, args, 4);
}

i32 irq_handler_set_notification(u32 handler, u32 ntfn, u32 ntfn_depth) {
    u32 args[] = {ntfn, ntfn_depth};
    return invoke(handler, CAPABILITY_LABEL_IRQ_SET_NTFN, args, 2);
}
