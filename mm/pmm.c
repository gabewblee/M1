#include <stdint.h>

#include "pmm.h"

#include "../boot/setup.h"
#include "../config.h"
#include "../drivers/vga.h"
#include "../lib/string.h"

#define _word_shl(x)       ((x) << 5)
#define _word_shr(x)       ((x) >> 5)
#define _ALIGN_UP_FRM(x)   (((x) + PG_SZ - 1) & ~(PG_SZ - 1))
#define _ALIGN_DOWN_FRM(x) ((x) & ~(PG_SZ - 1))
#define _MAX_NUM_FRMS      (1u << 20)
#define _NUM_WORDS         ((_MAX_NUM_FRMS >> 3) / sizeof(u32))

static u32 bitmap[_NUM_WORDS];
static u32 last;

extern u8  skernel[];
extern u8  ekernel[];
extern u8  sinit[];
extern u8  einit[];
extern u8  skheap[];
extern u8  ekheap[];
extern u32 swapper_pg_table_cnt;

static __always_inline void bitmap_mark_bit(u32 bit) {
    u32 word = _word_shr(bit), mask = one_shl(bit & 31);
    if (unlikely(bitmap[word] & mask))
        return;

    bitmap[word] |= mask;
}

static __always_inline void bitmap_unmark_bit(u32 bit) {
    u32 word = _word_shr(bit), mask = one_shl(bit & 31);
    if (unlikely(!(bitmap[word] & mask)))
        return;
    
    bitmap[word] &= ~mask;
}

static void bitmap_mark_range(phys_addr_t base, u32 len) {
    phys_addr_t start = _ALIGN_DOWN_FRM(base), end = _ALIGN_UP_FRM(base + len);
    for (phys_addr_t paddr = start; paddr < end; paddr += PG_SZ)
        bitmap_mark_bit(pg_shr(paddr));
}

static void bitmap_unmark_range(phys_addr_t base, u32 len) {
    phys_addr_t start = _ALIGN_UP_FRM(base), end = _ALIGN_DOWN_FRM(base + len);
    for (phys_addr_t paddr = start; paddr < end; paddr += PG_SZ)
        bitmap_unmark_bit(pg_shr(paddr));
}

static u32 find_unmarked_bit(u32 start, u32 end) {
    u32 first = _word_shr(start), last = _word_shr(end + 31);
    for (u32 word = first; word < last; word++) {
        u32 mask = ~bitmap[word];
        if (word == first) {
            u32 bit = start & 31;
            if (bit)
                mask &= ~(one_shl(bit) - 1);
        }

        if ((word + 1) == last) {
            u32 bit = end & 31;
            if (bit)
                mask &= (one_shl(bit) - 1);
        }

        if (!mask)
            continue;

        return (_word_shl(word)) + (u32)__builtin_ctz(mask);
    }
    return _MAX_NUM_FRMS;
}

static u32 find_unmarked_run(u32 hint, u32 cnt) {
    u32 i = hint, run = 0, run_start = 0, wrapped = 0;
    for (;;) {
        if (unlikely(i >= _MAX_NUM_FRMS)) {
            if (unlikely(wrapped)) break;
            wrapped = 1;
            i = 0;
            run = 0;
            continue;
        }
        if (unlikely(wrapped && i >= hint)) break;

        u32 word = bitmap[_word_shr(i)];
        if (unlikely(word == 0xFFFFFFFFu)) {
            run = 0;
            i = (i + 32) & ~31u;
            continue;
        }

        if (!(word & one_shl(i & 31))) {
            if (run == 0)
                run_start = i;

            run++;
            if (run >= cnt)
                return run_start;
        } else {
            run = 0;
        }
        i++;
    }
    return _MAX_NUM_FRMS;
}

void pmm_free_init_section(void) {
    bitmap_unmark_range(__pa(sinit), (u32)(einit - sinit));
}

phys_addr_t __hot pmm_alloc_frm(void) {
    u32 bit = find_unmarked_bit(last, _MAX_NUM_FRMS);
    if (unlikely(bit >= _MAX_NUM_FRMS && (bit = find_unmarked_bit(0, last)) >= _MAX_NUM_FRMS))
        return 0;

    bitmap_mark_bit(bit);
    last = (bit + 1) & (_MAX_NUM_FRMS - 1);
    return pg_shl(bit);
}

void __hot pmm_free_frm(phys_addr_t paddr) {
    bitmap_unmark_bit(pg_shr(paddr));
}

phys_addr_t __hot pmm_alloc_frms(u32 cnt) {
    if (cnt == 1)
        return pmm_alloc_frm();

    u32 first = find_unmarked_run(last, cnt);
    if (unlikely(first >= _MAX_NUM_FRMS))
        return 0;

    for (u32 i = 0; i < cnt; i++)
        bitmap_mark_bit(first + i);

    last = (first + cnt) & (_MAX_NUM_FRMS - 1);
    return pg_shl(first);
}

void __hot pmm_free_frms(phys_addr_t paddr, u32 cnt) {
    u32 first = pg_shr(paddr);
    for (u32 i = 0; i < cnt; i++)
        bitmap_unmark_bit(first + i);
}

void __init pmm_init(multiboot_info_t* mbinfo) {
    memset(bitmap, 0xFF, sizeof(bitmap));
    uintptr_t cur = __va(mbinfo->mmap_addr), end = cur + mbinfo->mmap_length;
    while (cur < end) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)cur;
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
            bitmap_unmark_range((phys_addr_t)mmap->addr, (u32)mmap->len);

        cur += mmap->size + sizeof(mmap->size);
    }

    /* Reserve null frame */
    bitmap_mark_range(0, PG_SZ);

    /* Reserve kernel and swapper page tables */
    bitmap_mark_range(__pa(skernel), (u32)(ekernel - skernel) + swapper_pg_table_cnt * PG_SZ);
}