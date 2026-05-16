#include "setup.h"

#define _PG_FLAGS     0x03u
#define _span_shl(x)  ((x) << 22u)
#define _span_shr(x)  ((x) >> 22u)

__aligned(PG_SZ)
u32 swapper_pg_dir[1024];
u32 swapper_pg_table_cnt;

extern u8 ekernel[];

void __multiboot setup_swapper_pg_dir(void) {
    u32 pg_table_cnt = _span_shr(__pa(ekernel) + _span_shl(1u) - 1u);
    if (pg_table_cnt == 0)
        for (;;) __asm__ volatile("hlt");

    phys_addr_t base = __pa(ekernel); u32* pg_table = (u32*)base;
    for (phys_addr_t paddr = 0; paddr < __pa(ekernel); paddr += PG_SZ)
        pg_table[pg_shr(paddr)] = paddr | _PG_FLAGS;

    u32* pg_dir = (u32*)__pa(swapper_pg_dir); u32 offset = _span_shr(HIGHER_HALF_OFFSET);
    for (u32 i = 0; i < pg_table_cnt; i++) {
        phys_addr_t paddr = base + (phys_addr_t)i * PG_SZ;
        pg_dir[i]          = paddr | _PG_FLAGS;
        pg_dir[offset + i] = paddr | _PG_FLAGS;
    }

    *(u32*)__pa(&swapper_pg_table_cnt) = pg_table_cnt;
}
