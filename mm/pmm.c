#include <stdint.h>

#include "bits.h"
#include "boot/multiboot.h"
#include "config.h"
#include "libk/string.h"
#include "mm/page.h"
#include "mm/pmm.h"

#define WORD_MAX_CNT (FRM_MAX_CNT >> 5) /* Maximum word count */

static u32 bitmap[WORD_MAX_CNT];
static u32 last;

extern const u8  skernel[];
extern const u8  ekernel[];
extern const u8  sinit[];
extern const u8  einit[];
extern const u32 swapper_pg_table_cnt;

static __always_inline void __hot bitmap_mark_bit(u32 bit) {
    const u32 word = BIT_TO_WORD(bit), mask = BIT(bit & 31);
    if (unlikely(bitmap[word] & mask))
        return;

    bitmap[word] |= mask;
}

static __always_inline void __hot bitmap_unmark_bit(u32 bit) {
    const u32 word = BIT_TO_WORD(bit), mask = BIT(bit & 31);
    if (unlikely(!(bitmap[word] & mask)))
        return;
    
    bitmap[word] &= ~mask;
}

static void bitmap_mark_range(phys_addr_t base, u32 len) {
    phys_addr_t start = ALIGN_DOWN_TO(base, PG_SZ), end = ALIGN_UP_TO(base + len, PG_SZ);
    for (phys_addr_t paddr = start; paddr < end; paddr += PG_SZ)
        bitmap_mark_bit(PHYS_TO_PFN(paddr));
}

static void bitmap_unmark_range(phys_addr_t base, u32 len) {
    phys_addr_t start = ALIGN_UP_TO(base, PG_SZ), end = ALIGN_DOWN_TO(base + len, PG_SZ);
    for (phys_addr_t paddr = start; paddr < end; paddr += PG_SZ)
        bitmap_unmark_bit(PHYS_TO_PFN(paddr));
}

static u32 find_unmarked_bit(u32 start, u32 end) {
    const u32 first = BIT_TO_WORD(start), last = BIT_TO_WORD(end + 31);
    for (u32 word = first; word < last; word++) {
        u32 mask = ~bitmap[word];
        if (word == first) {
            u32 bit = start & 31;
            if (bit)
                mask &= ~(BIT(bit) - 1);
        }

        if ((word + 1) == last) {
            u32 bit = end & 31;
            if (bit)
                mask &= (BIT(bit) - 1);
        }

        if (!mask)
            continue;

        return WORD_TO_BIT(word) + (u32)__builtin_ctz(mask);
    }
    return FRM_MAX_CNT;
}

static u32 find_unmarked_run(u32 hint, u32 cnt) {
    u32 i = hint, run = 0, run_start = 0, wrapped = 0;
    for (;;) {
        if (unlikely(i >= FRM_MAX_CNT)) {
            if (unlikely(wrapped)) break;
            wrapped = 1;
            i = 0;
            run = 0;
            continue;
        }
        if (unlikely(wrapped && i >= hint)) break;

        u32 word = bitmap[BIT_TO_WORD(i)];
        if (unlikely(word == 0xFFFFFFFFu)) {
            run = 0;
            i = ALIGN_DOWN_TO(i + 32, 32);
            continue;
        }

        if (!(word & BIT(i & 31))) {
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
    return FRM_MAX_CNT;
}

void pmm_free_init_section(void) {
    bitmap_unmark_range(__pa(sinit), (u32)(einit - sinit));
}

phys_addr_t pmm_alloc_frm(void) {
    u32 bit = find_unmarked_bit(last, FRM_MAX_CNT);
    if (unlikely(bit >= FRM_MAX_CNT && (bit = find_unmarked_bit(0, last)) >= FRM_MAX_CNT))
        return 0;

    bitmap_mark_bit(bit);
    last = (bit + 1) & (FRM_MAX_CNT - 1);
    return PFN_TO_PHYS(bit);
}

void pmm_free_frm(phys_addr_t paddr) {
    bitmap_unmark_bit(PHYS_TO_PFN(paddr));
}

phys_addr_t pmm_alloc_frms(u32 cnt) {
    if (cnt == 1)
        return pmm_alloc_frm();

    u32 first = find_unmarked_run(last, cnt);
    if (unlikely(first >= FRM_MAX_CNT))
        return 0;

    for (u32 i = 0; i < cnt; i++)
        bitmap_mark_bit(first + i);

    last = (first + cnt) & (FRM_MAX_CNT - 1);
    return PFN_TO_PHYS(first);
}

void pmm_free_frms(phys_addr_t paddr, u32 cnt) {
    u32 first = PHYS_TO_PFN(paddr);
    for (u32 i = 0; i < cnt; i++)
        bitmap_unmark_bit(first + i);
}

void __init pmm_init(const multiboot_info_t* mbinfo) {
    memset(bitmap, 0xFF, sizeof(bitmap));
    uintptr_t cur = __va(mbinfo->mmap_addr);
    const uintptr_t end = cur + mbinfo->mmap_length;
    while (cur < end) {
        const multiboot_memory_map_t* mmap = (const multiboot_memory_map_t*)cur;
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
            bitmap_unmark_range((phys_addr_t)mmap->addr, (u32)mmap->len);

        cur += mmap->size + sizeof(mmap->size);
    }

    /* Reserve null frame */
    bitmap_mark_range(0, PG_SZ);

    /* Reserve kernel and swapper page tables */
    bitmap_mark_range(__pa(skernel), (u32)(ekernel - skernel) + swapper_pg_table_cnt * PG_SZ);

    /* Reserve multiboot metadata */
    bitmap_mark_range(__pa(mbinfo), sizeof(multiboot_info_t));
    if (mbinfo->flags & MULTIBOOT_INFO_CMDLINE)
        bitmap_mark_range(mbinfo->cmdline, (u32)(strlen((const char*)__va(mbinfo->cmdline)) + 1));

    if (mbinfo->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
        bitmap_mark_range(mbinfo->boot_loader_name, (u32)(strlen((const char*)__va(mbinfo->boot_loader_name)) + 1));

    if (mbinfo->flags & MULTIBOOT_INFO_MEM_MAP)
        bitmap_mark_range(mbinfo->mmap_addr, mbinfo->mmap_length);

    /* Reserve multiboot modules */
    if (mbinfo->flags & MULTIBOOT_INFO_MODS) {
        bitmap_mark_range(mbinfo->mods_addr, mbinfo->mods_count * sizeof(multiboot_module_t));

        const multiboot_module_t* mods = (const multiboot_module_t*)__va(mbinfo->mods_addr);
        for (u32 i = 0; i < mbinfo->mods_count; i++) {
            if (mods[i].cmdline)
                bitmap_mark_range(mods[i].cmdline, (u32)(strlen((const char*)__va(mods[i].cmdline)) + 1));

            bitmap_mark_range(mods[i].mod_start, mods[i].mod_end - mods[i].mod_start);
        }
    }
}