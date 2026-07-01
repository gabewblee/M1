#include <bits.h>
#include <config.h>
#include <kernel/capability/capability.h>
#include <kernel/capability/endpoint.h>
#include <kernel/core/thread.h>
#include <libk/list.h>
#include <libk/string.h>
#include <stddef.h>
#include <mm.h>
#include <mm/page.h>
#include <mm/vmm.h>
#include <stddef.h>
#include <uapi/errno.h>

static inline u32 mask(u32 bits) {
    return bits >= CAPABILITY_CPTR_BITS ? ~0u : (1u << bits) - 1u;
}

static u32 get_obj_alignment(capability_type_e type, u32 nbits) {
    switch (type) {
        case CAPABILITY_TYPE_FRM:
        case CAPABILITY_TYPE_PG_TABLE:
        case CAPABILITY_TYPE_PG_DIR:
            return PG_SZ;
        case CAPABILITY_TYPE_UNTYPED:
            return 1u << nbits;
        default:
            return OBJ_MIN_ALIGN;
    }
}

static size_t get_obj_sz(capability_type_e type, u32 nbits) {
    switch (type) {
        case CAPABILITY_TYPE_UNTYPED:
            return (size_t)1u << nbits;
        case CAPABILITY_TYPE_CNODE:
            return ((size_t)1u << nbits) * sizeof(cte_s);
        case CAPABILITY_TYPE_TCB:
            return sizeof(thread_ctrl_blk_s);
        case CAPABILITY_TYPE_ENDPOINT:
            return sizeof(endpoint_s);
        case CAPABILITY_TYPE_NOTIFICATION:
            return sizeof(notification_s);
        case CAPABILITY_TYPE_REPLY:
            return OBJ_REPLY_SZ;
        case CAPABILITY_TYPE_FRM:
        case CAPABILITY_TYPE_PG_TABLE:
        case CAPABILITY_TYPE_PG_DIR:
            return PG_SZ;
        default:
            return 0;
    }
}

capability_s capability_mk_null(void) {
    return (capability_s){
        .type = CAPABILITY_TYPE_NULL,
    };
}

capability_s capability_mk_obj(capability_type_e type, void* obj, u32 rights, u32 badge) {
    return (capability_s){
        .type   = type,
        .obj    = obj,
        .rights = rights,
        .badge  = badge
    };
}

capability_s capability_mk_cnode(cte_s* slots, u8 radix, u32 guard, u8 guard_sz) {
    return (capability_s){
        .type     = CAPABILITY_TYPE_CNODE,
        .obj      = slots,
        .rights   = CAPABILITY_RIGHTS_ALL,
        .radix    = radix,
        .guard    = guard,
        .guard_sz = guard_sz
    };
}

capability_s capability_mk_untyped(void* base, u32 nbits) {
    return (capability_s){
        .type   = CAPABILITY_TYPE_UNTYPED,
        .obj    = base,
        .rights = CAPABILITY_RIGHTS_ALL,
        .radix  = (u8)nbits
    };
}

cte_s* capability_lookup(const capability_s* root, u32 cptr, u32 depth) {
    if (unlikely(!root || root->type != CAPABILITY_TYPE_CNODE || depth > CAPABILITY_CPTR_BITS))
        return NULL;

    capability_s node = *root;
    u32 rem = depth;
    for (;;) {
        u32 radix = node.radix, guard_sz = node.guard_sz, level = radix + guard_sz;
        if (unlikely(level == 0 || level > rem))
            return NULL;

        u32 guard = guard_sz ? ((cptr >> (rem - guard_sz)) & mask(guard_sz)) : 0u;
        if (unlikely(guard != node.guard))
            return NULL;

        u32 off = (cptr >> (rem - level)) & mask(radix);
        cte_s* slot = &((cte_s*)node.obj)[off];
        rem -= level;
        if (rem == 0)
            return slot;

        if (unlikely(slot->capability.type != CAPABILITY_TYPE_CNODE))
            return NULL;

        node = slot->capability;
    }
}


i32 capability_cp(cte_s* dst, cte_s* src) {
    if (unlikely(!dst || !src))
        return -(i32)E_INVAL;
    if (unlikely(dst->capability.type != CAPABILITY_TYPE_NULL))
        return -(i32)E_EXIST;
    if (unlikely(src->capability.type == CAPABILITY_TYPE_NULL))
        return -(i32)E_INVAL;

    dst->capability = src->capability;
    dst->mdb.parent = src->mdb.parent;

    list_init(&dst->mdb.children);
    list_init(&dst->mdb.sibling);
    if (src->mdb.parent)
        list_add_to_tail(&dst->mdb.sibling, &src->mdb.parent->mdb.children);

    return E_OK;
}

i32 capability_mint(cte_s* dst, cte_s* src, u32 rights, u32 badge) {
    if (unlikely(!dst || !src))
        return -(i32)E_INVAL;
    if (unlikely(dst->capability.type != CAPABILITY_TYPE_NULL))
        return -(i32)E_EXIST;
    if (unlikely(src->capability.type == CAPABILITY_TYPE_NULL))
        return -(i32)E_INVAL;

    dst->capability        = src->capability;
    dst->capability.rights = src->capability.rights & rights;
    dst->capability.badge  = badge;
    dst->mdb.parent        = src;

    list_init(&dst->mdb.children);
    list_init(&dst->mdb.sibling);
    list_add_to_tail(&dst->mdb.sibling, &src->mdb.children);
    return E_OK;
}

i32 capability_mv(cte_s* dst, cte_s* src) {
    if (unlikely(!dst || !src))
        return -(i32)E_INVAL;
    if (unlikely(dst->capability.type != CAPABILITY_TYPE_NULL))
        return -(i32)E_EXIST;
    if (unlikely(src->capability.type == CAPABILITY_TYPE_NULL))
        return -(i32)E_INVAL;

    dst->capability = src->capability;
    dst->mdb.parent = src->mdb.parent;

    list_init(&dst->mdb.sibling);
    if (src->mdb.parent)
        list_add_to_tail(&dst->mdb.sibling, &src->mdb.parent->mdb.children);

    list_init(&dst->mdb.children);
    while (!list_is_empty(&src->mdb.children)) {
        cte_s* child = list_first_entry(&src->mdb.children, cte_s, mdb.sibling);
        list_del(&child->mdb.sibling);
        child->mdb.parent = dst;
        list_add_to_tail(&child->mdb.sibling, &dst->mdb.children);
    }

    if (list_is_attached(&src->mdb.sibling))
        list_del(&src->mdb.sibling);

    cte_init(src);
    return E_OK;
}

i32 capability_rvk(cte_s* cte) {
    if (unlikely(!cte))
        return -(i32)E_INVAL;

    cte_s* child;
    cte_s* tmp;
    list_for_each_entry_safe(child, tmp, &cte->mdb.children, mdb.sibling)
        capability_del(child);

    if (cte->capability.type == CAPABILITY_TYPE_UNTYPED)
        cte->capability.used = 0;

    return E_OK;
}

i32 capability_del(cte_s* cte) {
    if (unlikely(!cte))
        return -(i32)E_INVAL;
    if (cte->capability.type == CAPABILITY_TYPE_NULL)
        return E_OK;

    capability_rvk(cte);
    if (list_is_attached(&cte->mdb.sibling))
        list_del(&cte->mdb.sibling);

    cte_init(cte);
    return E_OK;
}

i32 capability_transfer(const capability_s* src_root,
                        u32                 src_cptr, 
                        u32                 src_depth,
                        const capability_s* dst_root,
                        u32                 dst_cptr, 
                        u32                 dst_depth) {
    cte_s* src = capability_lookup(src_root, src_cptr, src_depth);
    if (unlikely(!src || src->capability.type == CAPABILITY_TYPE_NULL))
        return -(i32)E_INVAL;

    cte_s* dst = capability_lookup(dst_root, dst_cptr, dst_depth);
    if (unlikely(!dst))
        return -(i32)E_INVAL;

    return capability_mint(dst, src, src->capability.rights, src->capability.badge);
}

i32 capability_retype(cte_s* untyped, capability_type_e type, u32 nbits, cte_s* dst, u32 cnt) {
    if (unlikely(!untyped || !dst || cnt == 0))
        return -(i32)E_INVAL;
    if (unlikely(untyped->capability.type != CAPABILITY_TYPE_UNTYPED))
        return -(i32)E_INVAL;

    size_t sz = get_obj_sz(type, nbits);
    if (unlikely(sz == 0))
        return -(i32)E_INVAL;

    for (u32 i = 0; i < cnt; i++)
        if (unlikely(dst[i].capability.type != CAPABILITY_TYPE_NULL))
            return -(i32)E_EXIST;

    capability_s* u = &untyped->capability;
    u32 region = 1u << u->radix;
    u32 align = get_obj_alignment(type, nbits);
    u32 start = (u32)ALIGN_UP_TO(u->used, align);

    u32 total;
    if (unlikely(__builtin_mul_overflow((u32)sz, cnt, &total)))
        return -(i32)E_NOMEM;
    if (unlikely(start > region || total > region - start))
        return -(i32)E_NOMEM;

    u8* base = (u8*)u->obj;
    for (u32 i = 0; i < cnt; i++) {
        u8* mem = base + start + i * (u32)sz;
        memset(mem, 0, sz);

        capability_s capability;
        switch (type) {
            case CAPABILITY_TYPE_CNODE:
                cnode_init((cte_s*)mem, 1u << nbits);
                capability = capability_mk_cnode((cte_s*)mem, (u8)nbits, 0, 0);
                break;
            case CAPABILITY_TYPE_UNTYPED:
                capability = capability_mk_untyped(mem, nbits);
                break;
            case CAPABILITY_TYPE_ENDPOINT:
                endpoint_init((endpoint_s*)mem);
                capability = capability_mk_obj(type, mem, CAPABILITY_RIGHTS_ALL, 0);
                break;
            case CAPABILITY_TYPE_NOTIFICATION:
                notification_init((notification_s*)mem);
                capability = capability_mk_obj(type, mem, CAPABILITY_RIGHTS_ALL, 0);
                break;
            case CAPABILITY_TYPE_TCB:
                thread_obj_init((thread_ctrl_blk_s*)mem);
                capability = capability_mk_obj(type, mem, CAPABILITY_RIGHTS_ALL, 0);
                break;
            case CAPABILITY_TYPE_REPLY:
                reply_init((reply_s*)mem);
                capability = capability_mk_obj(type, mem, CAPABILITY_RIGHTS_ALL, 0);
                break;
            case CAPABILITY_TYPE_PG_DIR:
                /* Turn the zeroed frame into a usable VSpace and record its
                   physical address, which is what a page directory base is. */
                vmm_aspace_init(mem);
                capability = capability_mk_obj(type, (void*)__pa(mem), CAPABILITY_RIGHTS_ALL, 0);
                break;
            case CAPABILITY_TYPE_PG_TABLE:
                /* A page table is addressed by its physical base when installed
                   into a page directory entry. */
                capability = capability_mk_obj(type, (void*)__pa(mem), CAPABILITY_RIGHTS_ALL, 0);
                break;
            default:
                capability = capability_mk_obj(type, mem, CAPABILITY_RIGHTS_ALL, 0);
                break;
        }
        cte_insert_child(&dst[i], untyped, capability);
    }

    u->used = start + total;
    return E_OK;
}


void cte_insert_child(cte_s* dst, cte_s* parent, capability_s capability) {
    dst->capability = capability;
    dst->mdb.parent = parent;

    list_init(&dst->mdb.children);
    list_init(&dst->mdb.sibling);
    list_add_to_tail(&dst->mdb.sibling, &parent->mdb.children);
}

void cte_init(cte_s* cte) {
    cte->capability = capability_mk_null();
    cte->mdb.parent = NULL;
    
    list_init(&cte->mdb.children);
    list_init(&cte->mdb.sibling);
}

void cnode_init(cte_s* slots, u32 cnt) {
    for (u32 i = 0; i < cnt; i++)
        cte_init(&slots[i]);
}