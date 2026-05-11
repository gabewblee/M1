#include <stdint.h>

#include "pmm.h"

#include "../config.h"
#include "../drivers/vga.h"
#include "../lib/string.h"

#define __word_shl(x)         ((x) << 5)                              /* Multiply by 32 */
#define __word_shr(x)         ((x) >> 5)                              /* Divide by 32 */
#define __one_shl(x)          (1u << x)                               /* Shift 1 left x times */
#define PMM_FRM_ALIGN_UP(x)   (((x) + PG_SZ - 1) & ~(PG_SZ - 1))      /* Align up to frame boundary */
#define PMM_FRM_ALIGN_DOWN(x) ((x) & ~(PG_SZ - 1))                    /* Align down to frame boundary */
#define PMM_MAX_NUM_FRMS      (1u << 20)                              /* Maximum number of frames supported */
#define PMM_BITMAP_NUM_WORDS  ((PMM_MAX_NUM_FRMS >> 3) / sizeof(u32)) /* Number of words for the PMM bitmap */

static u32 bitmap[PMM_BITMAP_NUM_WORDS];                              /* PMM bitmap */
static u32 last;                                                      /* Last allocated frame index */
extern u8  _skernel[];                                                /* Start of the kernel in virtual memory */
extern u8  _ekernel[];                                                /* End of the kernel in virtual memory */
extern u8  _sinit[];                                                  /* Start of the init section in virtual memory */
extern u8  _einit[];                                                  /* End of the init section in virtual memory */
extern u8  _skheap[];                                                 /* Start of the kernel heap in virtual memory */
extern u8  _ekheap[];                                                 /* End of the kernel heap in virtual memory */

static __always_inline void pmm_bitmap_mark_bit(u32 bit) {
    bitmap[__word_shr(bit)] |= (__one_shl(bit & 31));
}

static __always_inline void pmm_bitmap_unmark_bit(u32 bit) {
    bitmap[__word_shr(bit)] &= ~(__one_shl(bit & 31));
}

static void pmm_bitmap_mark_range(phys_addr_t base, u32 len) {
    phys_addr_t start = PMM_FRM_ALIGN_DOWN(base), end = PMM_FRM_ALIGN_UP(base + len);
    for (phys_addr_t paddr = start; paddr < end; paddr += PG_SZ)
        pmm_bitmap_mark_bit(__pg_shr(paddr));
}

static void pmm_bitmap_unmark_range(phys_addr_t base, u32 len) {
    phys_addr_t start = PMM_FRM_ALIGN_UP(base), end = PMM_FRM_ALIGN_DOWN(base + len);
    for (phys_addr_t paddr = start; paddr < end; paddr += PG_SZ)
        pmm_bitmap_unmark_bit(__pg_shr(paddr));
}

static u32 pmm_find_unmarked_bit(u32 start, u32 end) {
    u32 first = __word_shr(start), last = __word_shr(end + 31);
    for (u32 word = first; word < last; word++) {
        u32 mask = ~bitmap[word];
        if (word == first) {
            u32 bit = start & 31;
            if (bit)
                mask &= ~(__one_shl(bit) - 1);
        }

        if ((word + 1) == last) {
            u32 bit = end & 31;
            if (bit)
                mask &= (__one_shl(bit) - 1);
        }

        if (!mask)
            continue;

        return (__word_shl(word)) + (u32)__builtin_ctz(mask);
    }
    return PMM_MAX_NUM_FRMS;
}

void pmm_free_init_section(void) {
    pmm_bitmap_unmark_range(__pa(_sinit), (u32)(_einit - _sinit));
}

phys_addr_t __hot pmm_alloc_frm(void) {
    u32 bit = pmm_find_unmarked_bit(last, PMM_MAX_NUM_FRMS);
    if (bit >= PMM_MAX_NUM_FRMS && (bit = pmm_find_unmarked_bit(0, last)) >= PMM_MAX_NUM_FRMS)
        return 0;

    pmm_bitmap_mark_bit(bit);
    last = (bit + 1) & (PMM_MAX_NUM_FRMS - 1);
    return __pg_shl(bit);
}

void __hot pmm_free_frm(phys_addr_t paddr) {
    if (!paddr)
        return;

    pmm_bitmap_unmark_bit(__pg_shr(paddr));
}

void __init pmm_init(multiboot_info_t* mbinfo) {
    memset(bitmap, 0xFF, sizeof(bitmap));
    uintptr_t cur = __va(mbinfo->mmap_addr), end = cur + mbinfo->mmap_length;
    while (cur < end) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)cur;
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
            pmm_bitmap_unmark_range((phys_addr_t)mmap->addr, (u32)mmap->len);

        cur += mmap->size + sizeof(mmap->size);
    }

    /* Reserve null page */
    pmm_bitmap_mark_range(0, PG_SZ);

    /* Reserve kernel */
    pmm_bitmap_mark_range(__pa(_skernel), (u32)(_ekernel - _skernel));

    /* Reserve kheap */
    pmm_bitmap_mark_range(__pa(_skheap), (u32)(_ekheap - _skheap));
}