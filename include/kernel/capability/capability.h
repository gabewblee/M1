#pragma once

#include <config.h>
#include <libk/list.h>
#include <stddef.h>
#include <uapi/capability.h>

#define CAPABILITY_CPTR_BIT_CNT 32u  /* Capability pointer bits  */
#define OBJ_MIN_ALIGN           16u  /* Minimum object alignment */
#define OBJ_ENDPOINT_SZ         64u  /* Endpoint object size     */
#define OBJ_NOTIFICATION_SZ     64u  /* Notification object size */
#define OBJ_TCB_SZ              256u /* TCB object size          */
#define OBJ_REPLY_SZ            32u  /* Reply object size        */

typedef u32 capability_type_e;

typedef struct capability_s {
    capability_type_e type;     /* Capability type          */
    void*             obj;      /* Capability object        */
    u32               rights;   /* Capability rights        */
    u32               badge;    /* Endpoint badge           */
    u8                radix;    /* log2(CNode/untyped size) */
    u8                guard_sz; /* CNode guard size in bits */
    u32               guard;    /* CNode guard value        */
    u32               used;     /* Untyped bytes used       */
} capability_s;

struct cte_s;

typedef struct mdb_node_s {
    struct cte_s* parent;   /* Parent capability           */
    list_node_s   sibling;  /* Parent's children list link */
    list_node_s   children; /* Children capability slots   */
} mdb_node_s;

typedef struct cte_s {
    capability_s capability; /* The slot's capability */
    mdb_node_s   mdb;        /* The slot's MDB node   */
} cte_s;

/**
 * capability_depth - Gets @capability's depth.
 * @capability: The CNode capability.
 * Returns: @capability's depth.
 */
static inline u32 capability_depth(const capability_s* capability) {
    return (u32)capability->radix + (u32)capability->guard_sz;
}

/**
 * capability_mk_null - Creates a null capability.
 * Returns: The capability of type CAPABILITY_TYPE_NULL.
 */
capability_s capability_mk_null(void);

/**
 * capability_mk_obj - Creates an object capability.
 * @type: The capability type.
 * @obj: The capability object.
 * @rights: The capability rights.
 * @badge: The endpoint badge.
 * Returns: The capability of type CAPABILITY_TYPE_OBJECT.
 */
capability_s capability_mk_obj(capability_type_e type, void* obj, u32 rights, u32 badge);

/**
 * capability_mk_cnode - Creates a CNode capability.
 * @slots: The CNode slot array.
 * @radix: log2(slot count).
 * @guard: The guard value.
 * @guard_sz: The guard size in bits.
 * Returns: The capability of type CAPABILITY_TYPE_CNODE.
 */
capability_s capability_mk_cnode(cte_s* slots, u8 radix, u32 guard, u8 guard_sz);

/**
 * capability_mk_untyped - Creates an untyped capability.
 * @base: The region base address.
 * @nbits: log2(region size) in bits.
 * Returns: The capability of type CAPABILITY_TYPE_UNTYPED.
 */
capability_s capability_mk_untyped(void* base, u32 nbits);

/**
 * capability_lookup - Looks up the capability slot associated with @cptr.
 * @root: The root CNode capability to lookup from.
 * @cptr: The capability pointer to lookup.
 * @depth: The number of bits from @cptr to lookup.
 * Returns: The capability slot on success, or NULL on failure.
 */
cte_s* capability_lookup(const capability_s* root, u32 cptr, u32 depth);

/**
 * capability_cp - Copies @src into empty slot @dst with equal rights.
 * @dst: The destination slot.
 * @src: The source slot.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_cp(cte_s* dst, cte_s* src);

/**
 * capability_mint - Derives a child of @src into empty slot @dst with diminished
 *                   @rights.
 * @dst: The destination slot.
 * @src: The source slot.
 * @rights: The rights to retain.
 * @badge: The endpoint badge.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_mint(cte_s* dst, cte_s* src, u32 rights, u32 badge);

/**
 * capability_mv - Moves @src into empty slot @dst, preserving rights.
 * @dst: The destination slot.
 * @src: The source slot.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_mv(cte_s* dst, cte_s* src);

/**
 * capability_rvk - Deletes every capability derived from @cte, excluding @cte itself.
 * @cte: The capability to revoke.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_rvk(cte_s* cte);

/**
 * capability_del - Deletes every capability derived from @cte, including @cte itself.
 * @cte: The capability to delete.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_del(cte_s* cte);

/**
 * capability_transfer - Transfers the source capability at @src_cptr to the destination
 *                       capability at @dst_cptr, preserving both rights and badge.
 * @src_root: The source CSpace root capability.
 * @src_cptr: The capability pointer to the source slot.
 * @src_depth: The number of bits from @src_cptr to lookup.
 * @dst_root: The destination CSpace root capability.
 * @dst_cptr: The capability pointer to the destination slot.
 * @dst_depth: The number of bits from @dst_cptr to lookup.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_transfer(const capability_s* src_root,
                        u32                 src_cptr,
                        u32                 src_depth,
                        const capability_s* dst_root,
                        u32                 dst_cptr,
                        u32                 dst_depth);

/**
 * capability_retype - Retypes @cnt objects from @untyped to @type in @dst.
 * @untyped: The untyped capability slot.
 * @type: The object type to retype to.
 * @nbits: log2(object size) in bits.
 * @dst: The destination capability slots.
 * @cnt: The number of objects to retype.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 capability_retype(cte_s* untyped, capability_type_e type, u32 nbits, cte_s* dst, u32 cnt);

/**
 * cte_insert_child - Inserts @dst into @parent's children list.
 * @dst: The child slot.
 * @parent: The parent slot.
 * @capability: The child capability.
 */
void cte_insert_child(cte_s* dst, cte_s* parent, capability_s capability);

/**
 * cte_init - Initializes @cte as an empty slot.
 * @cte: The slot to initialize.
 */
void cte_init(cte_s* cte);

/**
 * cnode_init - Initializes @cnt CNode slots.
 * @slots: The CNode slots.
 * @cnt: The slot count.
 */
void cnode_init(cte_s* slots, u32 cnt);