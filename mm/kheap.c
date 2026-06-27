#include <bits.h>
#include <kernel/core/initcall.h>
#include <libk/string.h>
#include <libk/list.h>
#include <mm/kheap.h>
#include <stdbool.h>

#define MIN_BLK_SZ (sizeof(kheap_blk_hdr_s) + sizeof(kheap_blk_ftr_s) + 16) /* Minimum block size */

typedef struct kheap_blk_hdr_s {
    size_t      sz;   /* Block size         */
    bool        free; /* Block availability */
    list_node_s link; /* Free list node     */ 
} kheap_blk_hdr_s;

typedef struct kheap_blk_ftr_s {
    size_t           sz;  /* Block size           */
    kheap_blk_hdr_s* hdr; /* Block header pointer */
} kheap_blk_ftr_s;

static list_node_s free_list;

extern u8 skheap[];
extern u8 ekheap[];

static inline kheap_blk_hdr_s* get_hdr(kheap_blk_ftr_s* ftr) {
    return ftr->hdr;
}

static inline void set_hdr(kheap_blk_hdr_s* hdr, size_t sz, bool free) {
    hdr->sz   = sz;
    hdr->free = free;
}

static inline kheap_blk_ftr_s* get_ftr(kheap_blk_hdr_s* hdr) {
    return (kheap_blk_ftr_s*)((uintptr_t)hdr + hdr->sz - sizeof(kheap_blk_ftr_s));
}

static inline void set_ftr(kheap_blk_hdr_s* hdr) {
    kheap_blk_ftr_s* ftr = get_ftr(hdr);
    ftr->sz                     = hdr->sz;
    ftr->hdr                    = hdr;
}

static inline void blk_init(kheap_blk_hdr_s* hdr, size_t sz, bool free) {
    set_hdr(hdr, sz, free);
    list_init(&hdr->link);
    set_ftr(hdr);
}

static inline kheap_blk_hdr_s* get_nxt(kheap_blk_hdr_s* hdr) {
    return (kheap_blk_hdr_s*)((u8*)hdr + hdr->sz);
}

static inline bool has_nxt(kheap_blk_hdr_s* hdr) {
    return (u8*)get_nxt(hdr) < ekheap;
}

static inline kheap_blk_hdr_s* get_prv(kheap_blk_hdr_s* hdr) {
    kheap_blk_ftr_s* ftr = (kheap_blk_ftr_s*)((u8*)hdr - sizeof(kheap_blk_ftr_s));
    return get_hdr(ftr);
}

static inline bool has_prv(kheap_blk_hdr_s* hdr) {
    return (u8*)hdr > skheap;
}

static inline void add_to_free_list(kheap_blk_hdr_s* hdr) {
    list_add_to_head(&hdr->link, &free_list);
}

static inline void remove_from_free_list(kheap_blk_hdr_s* hdr) {
    list_del(&hdr->link);
}

static inline void merge_with_nxt(kheap_blk_hdr_s* hdr) {
    kheap_blk_hdr_s* nxt = get_nxt(hdr);
    remove_from_free_list(nxt);

    hdr->sz += nxt->sz;
    set_ftr(hdr);
}

static inline kheap_blk_hdr_s* merge_with_prv(kheap_blk_hdr_s* hdr) {
    kheap_blk_hdr_s* prv = get_prv(hdr);
    remove_from_free_list(prv);

    prv->sz += hdr->sz;
    set_ftr(prv);
    return prv;
}

static inline size_t get_req_blk_sz(size_t sz) {
    size_t req = ALIGN_UP_TO(sz, 8) + sizeof(kheap_blk_hdr_s) + sizeof(kheap_blk_ftr_s);
    if (req < MIN_BLK_SZ)
        req = MIN_BLK_SZ;

    return req;
}

static inline kheap_blk_hdr_s* coalesce(kheap_blk_hdr_s* hdr) {
    if (has_nxt(hdr)) {
        kheap_blk_hdr_s* nxt = get_nxt(hdr);
        if (nxt->free)
            merge_with_nxt(hdr);
    }

    if (has_prv(hdr)) {
        kheap_blk_hdr_s* prv = get_prv(hdr);
        if (prv->free)
            hdr = merge_with_prv(hdr);
    }

    return hdr;
}

static inline void split_blk(kheap_blk_hdr_s* hdr, size_t sz) {
    size_t remaining = hdr->sz - sz;
    if (remaining < MIN_BLK_SZ)
        return;

    hdr->sz = sz;
    set_ftr(hdr);

    kheap_blk_hdr_s* new = get_nxt(hdr);
    blk_init(new, remaining, true);
    add_to_free_list(new);
}

void* kzalloc(size_t sz) {
    void* ptr = kmalloc(sz);
    if (likely(ptr))
        memset(ptr, 0, sz);

    return ptr;
}

void* kmalloc(size_t sz) {
    size_t req = get_req_blk_sz(sz);
    kheap_blk_hdr_s* cur;
    list_for_each_entry(cur, &free_list, link) {
        if (cur->free && cur->sz >= req) {
            remove_from_free_list(cur);
            split_blk(cur, req);
            cur->free = false;
            return (u8*)cur + sizeof(kheap_blk_hdr_s);
        }
    }

    return NULL;
}

void kfree(void* ptr) {
    kheap_blk_hdr_s* hdr = (kheap_blk_hdr_s*)((u8*)ptr - sizeof(kheap_blk_hdr_s));
    hdr->free                   = true;
    hdr                         = coalesce(hdr);
    add_to_free_list(hdr);
}

void __init kheap_init(void) {
    size_t kheap_sz = ekheap - skheap;
    kheap_blk_hdr_s* hdr = (kheap_blk_hdr_s*)skheap;
    list_init(&free_list);
    blk_init(hdr, kheap_sz, true);
    add_to_free_list(hdr);
}

core_initcall(kheap_init);