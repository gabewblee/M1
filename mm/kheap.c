#include <stdbool.h>

#include "kheap.h"

#define KHEAP_MIN_BLK_SZ    (sizeof(kheap_blk_hdr_t) + sizeof(kheap_blk_ftr_t) + 16) /* Minimum size of a kernel heap block */
#define KHEAP_ALIGN_UP_8(x) ((x + 7) & ~7)                                           /* Align up to 8-byte boundary */

typedef struct kheap_blk_hdr_t {
    size_t                  sz;                                                      /* Size of the kernel heap block */
    bool                    free;                                                    /* Whether the kernel heap block is free */
    struct kheap_blk_hdr_t* nxt;                                                     /* Next block in the free list */
    struct kheap_blk_hdr_t* prv;                                                     /* Previous block in the free list */
} kheap_blk_hdr_t;

typedef struct kheap_blk_ftr_t {
    size_t           sz;                                                             /* Size of the kernel heap block */
    kheap_blk_hdr_t* hdr;                                                            /* Header of the kernel heap block */
} kheap_blk_ftr_t;

static kheap_blk_hdr_t* kheap_free_list;                                             /* Free list of kernel heap blocks */
extern u8               _skheap[];                                                   /* Start of the kernel heap in virtual memory */
extern u8               _ekheap[];                                                   /* End of the kernel heap in virtual memory */

static inline kheap_blk_hdr_t* kheap_get_hdr(kheap_blk_ftr_t* ftr) {
    return ftr->hdr;
}

static inline void kheap_set_hdr(kheap_blk_hdr_t* hdr, size_t sz, bool free, kheap_blk_hdr_t* nxt, kheap_blk_hdr_t* prv) {
    hdr->sz   = sz;
    hdr->free = free;
    hdr->nxt  = nxt;
    hdr->prv  = prv;
}

static inline kheap_blk_ftr_t* kheap_get_ftr(kheap_blk_hdr_t* hdr) {
    return (kheap_blk_ftr_t*)((uintptr_t)hdr + hdr->sz - sizeof(kheap_blk_ftr_t));
}

static inline void kheap_set_ftr(kheap_blk_hdr_t* hdr) {
    kheap_blk_ftr_t* ftr = kheap_get_ftr(hdr);
    ftr->sz              = hdr->sz;
    ftr->hdr             = hdr;
}

static inline void kheap_blk_init(kheap_blk_hdr_t* hdr, size_t sz, bool free) {
    kheap_set_hdr(hdr, sz, free, NULL, NULL);
    kheap_set_ftr(hdr);
}

static inline kheap_blk_hdr_t* kheap_get_nxt(kheap_blk_hdr_t* hdr) {
    return (kheap_blk_hdr_t*)((u8*)hdr + hdr->sz);
}

static inline bool kheap_has_nxt(kheap_blk_hdr_t* hdr) {
    return (u8*)kheap_get_nxt(hdr) < _ekheap;
}

static inline kheap_blk_hdr_t* kheap_get_prv(kheap_blk_hdr_t* hdr) {
    kheap_blk_ftr_t* ftr = (kheap_blk_ftr_t*)((u8*)hdr - sizeof(kheap_blk_ftr_t));
    return kheap_get_hdr(ftr);
}

static inline bool kheap_has_prv(kheap_blk_hdr_t* hdr) {
    return (u8*)hdr > _skheap;
}

static void kheap_add_to_free_list(kheap_blk_hdr_t* hdr) {
    hdr->nxt = kheap_free_list;
    hdr->prv = NULL;
    if (kheap_free_list)
        kheap_free_list->prv = hdr;

    kheap_free_list = hdr;
}

static void kheap_remove_from_free_list(kheap_blk_hdr_t* hdr) {
    if (hdr->prv)
        hdr->prv->nxt = hdr->nxt;
    else
        kheap_free_list = hdr->nxt;

    if (hdr->nxt)
        hdr->nxt->prv = hdr->prv;

    hdr->nxt = NULL;
    hdr->prv = NULL;
}

static void kheap_merge_with_nxt(kheap_blk_hdr_t* hdr) {
    kheap_blk_hdr_t* nxt = kheap_get_nxt(hdr);
    kheap_remove_from_free_list(nxt);

    hdr->sz += nxt->sz;
    kheap_set_ftr(hdr);
}

static kheap_blk_hdr_t* kheap_merge_with_prv(kheap_blk_hdr_t* hdr) {
    kheap_blk_hdr_t* prv = kheap_get_prv(hdr);
    kheap_remove_from_free_list(prv);

    prv->sz += hdr->sz;
    kheap_set_ftr(prv);
    return prv;
}

static size_t kheap_get_req_blk_sz(size_t sz) {
    size_t req = KHEAP_ALIGN_UP_8(sz) + sizeof(kheap_blk_hdr_t) + sizeof(kheap_blk_ftr_t);
    if (req < KHEAP_MIN_BLK_SZ)
        req = KHEAP_MIN_BLK_SZ;

    return req;
}

static kheap_blk_hdr_t* kheap_coalesce(kheap_blk_hdr_t* hdr) {
    if (kheap_has_nxt(hdr)) {
        kheap_blk_hdr_t* nxt = kheap_get_nxt(hdr);
        if (nxt->free)
            kheap_merge_with_nxt(hdr);
    }

    if (kheap_has_prv(hdr)) {
        kheap_blk_hdr_t* prv = kheap_get_prv(hdr);
        if (prv->free)
            hdr = kheap_merge_with_prv(hdr);
    }

    return hdr;
}

static void kheap_split_blk(kheap_blk_hdr_t* hdr, size_t sz) {
    size_t remaining = hdr->sz - sz;
    if (remaining < KHEAP_MIN_BLK_SZ)
        return;

    hdr->sz = sz;
    kheap_set_ftr(hdr);

    kheap_blk_hdr_t* new = kheap_get_nxt(hdr);
    kheap_blk_init(new, remaining, true);
    kheap_add_to_free_list(new);
}

void* __hot kmalloc(size_t sz) {
    size_t req = kheap_get_req_blk_sz(sz);
    for (kheap_blk_hdr_t* cur = kheap_free_list; cur; cur = cur->nxt) {
        if (cur->free && cur->sz >= req) {
            kheap_remove_from_free_list(cur);
            kheap_split_blk(cur, req);
            cur->free = false;
            return (u8*)cur + sizeof(kheap_blk_hdr_t);
        }
    }

    return NULL;
}

void __hot kfree(void* ptr) {
    if (!ptr)
        return;

    kheap_blk_hdr_t* hdr = (kheap_blk_hdr_t*)((u8*)ptr - sizeof(kheap_blk_hdr_t));
    hdr->free            = true;
    hdr                  = kheap_coalesce(hdr);
    kheap_add_to_free_list(hdr);
}

void __init kheap_init(void) {
    size_t kheap_sz = _ekheap - _skheap;
    kheap_blk_hdr_t* hdr = (kheap_blk_hdr_t*)_skheap;
    kheap_blk_init(hdr, kheap_sz, true);
    kheap_free_list = hdr;
}