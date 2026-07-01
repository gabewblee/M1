#include <kernel/capability/capability.h>
#include <kernel/capability/endpoint.h>
#include <kernel/capability/invoke.h>
#include <kernel/core/thread.h>
#include <kernel/irq/irq.h>
#include <mm.h>
#include <mm/page.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <uapi/errno.h>

static i32 invoke_retype(const capability_s* root, cte_s* untyped, const u32* args) {
    cte_s* cnode = capability_lookup(root, args[2], args[3]);
    if (unlikely(!cnode || cnode->capability.type != CAPABILITY_TYPE_CNODE))
        return -(i32)E_INVAL;

    u32 slots = 1u << cnode->capability.radix;
    u32 index = args[4];
    u32 count = args[5];
    if (unlikely(index >= slots || count > slots - index))
        return -(i32)E_INVAL;

    cte_s* dst = &((cte_s*)cnode->capability.obj)[index];
    return capability_retype(untyped, (capability_type_e)args[0], args[1], dst, count);
}

static i32 invoke_tcb_configure(const capability_s* root,
                                cte_s*              tcb,
                                const u32*          args,
                                thread_ctrl_blk_s*  cur) {
    if (unlikely(tcb->capability.type != CAPABILITY_TYPE_TCB))
        return -(i32)E_INVAL;

    cte_s* cspace = capability_lookup(root, args[0], args[1]);
    cte_s* vspace = capability_lookup(root, args[2], args[3]);
    if (unlikely(!cspace || cspace->capability.type != CAPABILITY_TYPE_CNODE))
        return -(i32)E_INVAL;
    if (unlikely(!vspace || vspace->capability.type != CAPABILITY_TYPE_PG_DIR))
        return -(i32)E_INVAL;

    bool user = args[7] & 1u;
    return thread_obj_configure(
        (thread_ctrl_blk_s*)tcb->capability.obj,
        cur->task,
        (virt_addr_t)args[4],
        (virt_addr_t)args[5],
        (phys_addr_t)vspace->capability.obj,
        cspace->capability,
        (u8)args[6],
        user
    );
}

static i32 invoke_irq_get(const capability_s* root, cte_s* control, const u32* args) {
    if (unlikely(control->capability.type != CAPABILITY_TYPE_IRQ_CONTROL))
        return -(i32)E_INVAL;

    cte_s* cnode = capability_lookup(root, args[1], args[2]);
    if (unlikely(!cnode || cnode->capability.type != CAPABILITY_TYPE_CNODE))
        return -(i32)E_INVAL;
    if (unlikely(args[3] >= (1u << cnode->capability.radix)))
        return -(i32)E_INVAL;

    cte_s* dst = &((cte_s*)cnode->capability.obj)[args[3]];
    if (unlikely(dst->capability.type != CAPABILITY_TYPE_NULL))
        return -(i32)E_EXIST;

    cte_insert_child(
        dst,
        control,
        capability_mk_obj(CAPABILITY_TYPE_IRQ_HANDLER, NULL, CAPABILITY_RIGHTS_ALL, args[0])
    );
    return E_OK;
}

static i32 invoke_irq_set_ntfn(const capability_s* root, cte_s* handler, const u32* args) {
    if (unlikely(handler->capability.type != CAPABILITY_TYPE_IRQ_HANDLER))
        return -(i32)E_INVAL;

    cte_s* ntfn = capability_lookup(root, args[0], args[1]);
    if (unlikely(!ntfn || ntfn->capability.type != CAPABILITY_TYPE_NOTIFICATION))
        return -(i32)E_INVAL;

    return irq_bind_notification((u8)handler->capability.badge, (notification_s*)ntfn->capability.obj);
}

static i32 invoke_map_pg(const capability_s* root, cte_s* frame, const u32* args) {
    if (unlikely(frame->capability.type != CAPABILITY_TYPE_FRM))
        return -(i32)E_INVAL;

    cte_s* vspace = capability_lookup(root, args[0], args[1]);
    if (unlikely(!vspace || vspace->capability.type != CAPABILITY_TYPE_PG_DIR))
        return -(i32)E_INVAL;

    virt_addr_t vaddr = (virt_addr_t)args[2];
    if (unlikely(vaddr >= HIGHER_HALF_OFFSET || (vaddr & (PG_SZ - 1u))))
        return -(i32)E_INVAL;

    u32 flags = PG_USER_FLAG | (args[3] & PG_RW_FLAG);
    return vmm_map_frm_in((phys_addr_t)vspace->capability.obj, __pa(frame->capability.obj), vaddr, flags);
}

static i32 invoke_unmap_pg(const capability_s* root, cte_s* frame, const u32* args) {
    if (unlikely(frame->capability.type != CAPABILITY_TYPE_FRM))
        return -(i32)E_INVAL;

    cte_s* vspace = capability_lookup(root, args[0], args[1]);
    if (unlikely(!vspace || vspace->capability.type != CAPABILITY_TYPE_PG_DIR))
        return -(i32)E_INVAL;

    virt_addr_t vaddr = (virt_addr_t)args[2];
    if (unlikely(vaddr >= HIGHER_HALF_OFFSET || (vaddr & (PG_SZ - 1u))))
        return -(i32)E_INVAL;

    vmm_unmap_pg_in((phys_addr_t)vspace->capability.obj, vaddr);
    return E_OK;
}

static i32 invoke_map_pg_table(const capability_s* root, cte_s* table, const u32* args) {
    if (unlikely(table->capability.type != CAPABILITY_TYPE_PG_TABLE))
        return -(i32)E_INVAL;

    cte_s* vspace = capability_lookup(root, args[0], args[1]);
    if (unlikely(!vspace || vspace->capability.type != CAPABILITY_TYPE_PG_DIR))
        return -(i32)E_INVAL;

    virt_addr_t vaddr = (virt_addr_t)args[2];
    if (unlikely(vaddr >= HIGHER_HALF_OFFSET))
        return -(i32)E_INVAL;

    return vmm_map_pg_table_in((phys_addr_t)vspace->capability.obj,
                               (phys_addr_t)table->capability.obj, vaddr, PG_USER_FLAG);
}

i32 capability_invoke(thread_ctrl_blk_s* cur, cte_s* slot, u32 label, const u32* args, u32 argc) {
    if (unlikely(argc < CAPABILITY_INVOKE_ARGC))
        return -(i32)E_INVAL;

    const capability_s* root = &cur->cspace_root;
    switch (label) {
        case CAPABILITY_LABEL_UNTYPED_RETYPE:
            return invoke_retype(root, slot, args);
        case CAPABILITY_LABEL_CNODE_COPY:
        case CAPABILITY_LABEL_CNODE_MINT:
        case CAPABILITY_LABEL_CNODE_MOVE: {
            if (unlikely(slot->capability.type != CAPABILITY_TYPE_CNODE))
                return -(i32)E_INVAL;

            cte_s* dst = capability_lookup(&slot->capability, args[0], args[1]);
            cte_s* src = capability_lookup(&slot->capability, args[2], args[3]);
            if (unlikely(!dst || !src))
                return -(i32)E_INVAL;

            if (label == CAPABILITY_LABEL_CNODE_COPY)
                return capability_cp(dst, src);
            if (label == CAPABILITY_LABEL_CNODE_MINT)
                return capability_mint(dst, src, args[4], args[5]);
            return capability_mv(dst, src);
        }
        case CAPABILITY_LABEL_CNODE_DELETE:
        case CAPABILITY_LABEL_CNODE_REVOKE: {
            if (unlikely(slot->capability.type != CAPABILITY_TYPE_CNODE))
                return -(i32)E_INVAL;

            cte_s* target = capability_lookup(&slot->capability, args[0], args[1]);
            if (unlikely(!target))
                return -(i32)E_INVAL;

            return label == CAPABILITY_LABEL_CNODE_DELETE ? capability_del(target) : capability_rvk(target);
        }
        case CAPABILITY_LABEL_TCB_CONFIGURE:
            return invoke_tcb_configure(root, slot, args, cur);
        case CAPABILITY_LABEL_TCB_RESUME:
            if (unlikely(slot->capability.type != CAPABILITY_TYPE_TCB))
                return -(i32)E_INVAL;
            thread_obj_resume((thread_ctrl_blk_s*)slot->capability.obj);
            return E_OK;
        case CAPABILITY_LABEL_PAGE_MAP:
            return invoke_map_pg(root, slot, args);
        case CAPABILITY_LABEL_PAGE_UNMAP:
            return invoke_unmap_pg(root, slot, args);
        case CAPABILITY_LABEL_PAGE_TABLE_MAP:
            return invoke_map_pg_table(root, slot, args);
        case CAPABILITY_LABEL_IRQ_GET:
            return invoke_irq_get(root, slot, args);
        case CAPABILITY_LABEL_IRQ_SET_NTFN:
            return invoke_irq_set_ntfn(root, slot, args);
        default:
            return -(i32)E_NOSYS;
    }
}
