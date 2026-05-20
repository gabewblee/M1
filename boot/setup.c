#include "boot/setup.h"
#include "mm/page.h"

#define PG_FLAGS      0x03u        /* Present and writable flags  */
#define SPAN_TO_SZ(x) ((x) << 22u) /* Converts span count to size */
#define SZ_TO_SPAN(x) ((x) >> 22u) /* Converts size to span count */

__aligned(PG_SZ)
u32 swapper_pg_dir[1024];
u32 swapper_pg_table_cnt;

extern u8 ekernel[];

void __multiboot setup_swapper_pg_dir(void) {
    u32 pg_table_cnt = SZ_TO_SPAN(__pa(ekernel) + SPAN_TO_SZ(1u) - 1u);
    if (pg_table_cnt == 0)
        for (;;) __asm__ volatile("hlt");

    phys_addr_t base = __pa(ekernel); u32* pg_table = (u32*)base;
    for (phys_addr_t paddr = 0; paddr < __pa(ekernel); paddr += PG_SZ)
        pg_table[PHYS_TO_PFN(paddr)] = paddr | PG_FLAGS;

    u32* pg_dir = (u32*)__pa(swapper_pg_dir); u32 offset = SZ_TO_SPAN(HIGHER_HALF_OFFSET);
    for (u32 i = 0; i < pg_table_cnt; i++) {
        phys_addr_t paddr = base + (phys_addr_t)i * PG_SZ;
        pg_dir[i] = pg_dir[offset + i] = paddr | PG_FLAGS;
    }

    *(u32*)__pa(&swapper_pg_table_cnt) = pg_table_cnt;
}
