#include <boot/setup.h>
#include <mm/page.h>
#include <uapi/mm.h>

#define SPAN_CNT_TO_BYTE_CNT(x) ((x) << 22u)                  /* Converts span count to byte count             */
#define BYTE_CNT_TO_SPAN_CNT(x) ((x) >> 22u)                  /* Converts byte count to span count             */
#define PG_TABLE_IDX(paddr)     (PHYS_TO_PFN(paddr) & 0x3FFu) /* Converts physical address to page table index */

__aligned(PG_SZ)
u32 swapper_pg_dir[PG_ENTRY_CNT];
u32 swapper_pg_table_cnt;

extern u8 ekernel[];

void __multiboot setup_swapper_pg_dir(void) {
    u32 pg_table_cnt = BYTE_CNT_TO_SPAN_CNT(__pa(ekernel) + SPAN_CNT_TO_BYTE_CNT(1u) - 1u);
    if (pg_table_cnt == 0)
        for (;;) __asm__ volatile("hlt");

    /* ekernel must be 4KiB aligned since page tables require 4KiB alignment */
    phys_addr_t base = __pa(ekernel); u32* pg_table = (u32*)base;
    for (phys_addr_t paddr = 0; paddr < __pa(ekernel); paddr += PG_SZ)
        pg_table[PG_TABLE_IDX(paddr)] = paddr | PG_RW_FLAG | PG_PRESENT_FLAG;

    u32* pg_dir = (u32*)__pa(swapper_pg_dir); u32 offset = BYTE_CNT_TO_SPAN_CNT(HIGHER_HALF_OFFSET);
    for (u32 i = 0; i < pg_table_cnt; i++) {
        phys_addr_t paddr = base + (phys_addr_t)i * PG_SZ;

        /* Identity and higher-half mapping */
        pg_dir[i] = pg_dir[offset + i] = paddr | PG_RW_FLAG | PG_PRESENT_FLAG;
    }

    *(u32*)__pa(&swapper_pg_table_cnt) = pg_table_cnt;
}