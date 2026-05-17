#include <stdbool.h>

#include "pmm.h"
#include "vmm.h"

#include "../boot/setup.h"
#include "../config.h"
#include "../kernel/panic.h"
#include "../lib/string.h"

#define _PG_RW_FLAG                 0x02
#define _PG_USER_FLAG               0x04
#define _PG_GLOBAL_FLAG             0x100
#define _PG_DIR_SELF_ENTRY          1023u
#define _get_pg_table(pg_table_idx) ((virt_addr_t)((_PG_DIR_SELF_ENTRY << 22u) | ((pg_table_idx) << 12u)))
#define _kernel_pg_dir              (*(pg_dir_t*)swapper_pg_dir)

typedef struct pg_dir_entry_t {
    u8          present   : 1;
    u8          rw        : 1;
    u8          user      : 1;
    u8          pwt       : 1;
    u8          pcd       : 1;
    u8          accessed  : 1;
    u8          lower_avl : 1;
    u8          ps        : 1;
    u8          upper_avl : 4;
    phys_addr_t paddr     : 20;
} __packed pg_dir_entry_t;

typedef struct pg_table_entry_t {
    u8          present  : 1;
    u8          rw       : 1;
    u8          user     : 1;
    u8          pwt      : 1;
    u8          pcd      : 1;
    u8          accessed : 1;
    u8          dirty    : 1;
    u8          pat      : 1;
    u8          global   : 1;
    u8          avl      : 3;
    phys_addr_t paddr    : 20;
} __packed pg_table_entry_t;

typedef struct pg_dir_t {
    pg_dir_entry_t entries[1024];
} __aligned(PG_SZ) pg_dir_t;

typedef struct pg_table_t {
    pg_table_entry_t entries[1024];
} __aligned(PG_SZ) pg_table_t;

extern u32 swapper_pg_dir[1024];

static inline u32 get_pg_dir_idx(virt_addr_t vaddr) {
    return vaddr >> 22;
}

static inline pg_dir_entry_t* get_pg_dir_entry(u32 pg_dir_idx) {
    return &_kernel_pg_dir.entries[pg_dir_idx];
}

static inline void set_pg_dir_entry(pg_dir_entry_t* entry, phys_addr_t paddr, u32 flags) {
    entry->present = 1;
    entry->rw      = 1;
    entry->user    = (flags & _PG_USER_FLAG) ? 1 : 0;
    entry->paddr   = pg_shr(paddr);
}

static inline u32 get_pg_table_idx(virt_addr_t vaddr) {
    return (vaddr >> 12) & 0x3FF;
}

static inline pg_table_entry_t* get_pg_table_entry(u32 pg_dir_idx, u32 pg_table_idx) {
    pg_table_t* pg_table = (pg_table_t*)_get_pg_table(pg_dir_idx);
    return &pg_table->entries[pg_table_idx];
}

static inline void set_pg_table_entry(pg_table_entry_t* entry, phys_addr_t paddr, u32 flags) {
    entry->present = 1;
    entry->rw      = (flags & _PG_RW_FLAG) ? 1 : 0;
    entry->user    = (flags & _PG_USER_FLAG) ? 1 : 0;
    entry->global  = (flags & _PG_GLOBAL_FLAG) ? 1 : 0;
    entry->paddr   = pg_shr(paddr);
}

static inline void flush_tlb_mapping(virt_addr_t vaddr) {
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

void vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags) {
    u32 pg_dir_idx = get_pg_dir_idx(vaddr);
    pg_dir_entry_t* pg_dir_entry = get_pg_dir_entry(pg_dir_idx);
    if (!pg_dir_entry->present) {
        phys_addr_t paddr = pmm_alloc_frm();
        if (unlikely(!paddr))
            PANIC("Error: Failed to allocate frame");

        set_pg_dir_entry(pg_dir_entry, paddr, flags);
        memset((pg_table_t*)_get_pg_table(pg_dir_idx), 0, sizeof(pg_table_t));
    }

    u32 pg_table_idx = get_pg_table_idx(vaddr);
    pg_table_entry_t* pg_table_entry = get_pg_table_entry(pg_dir_idx, pg_table_idx);
    set_pg_table_entry(pg_table_entry, paddr, flags);
    flush_tlb_mapping(vaddr);
}

void vmm_unmap_pg(virt_addr_t vaddr) {
    u32 pg_dir_idx = get_pg_dir_idx(vaddr);
    pg_dir_entry_t* pg_dir_entry = get_pg_dir_entry(pg_dir_idx);
    if (unlikely(!pg_dir_entry->present))
        return;

    u32 pg_table_idx = get_pg_table_idx(vaddr);
    pg_table_entry_t* pg_table_entry = get_pg_table_entry(pg_dir_idx, pg_table_idx);
    pg_table_entry->present = 0;
    flush_tlb_mapping(vaddr);
}

void vmm_demand_map(virt_addr_t vaddr) {
    phys_addr_t paddr = pmm_alloc_frm();
    if (unlikely(!paddr))
        PANIC("Error: Failed to allocate frame");

    vmm_map_pg(paddr, vaddr, _PG_RW_FLAG);
}

void __init vmm_init(void) {
    pg_dir_entry_t* pg_dir_entry = get_pg_dir_entry(_PG_DIR_SELF_ENTRY);
    set_pg_dir_entry(pg_dir_entry, __pa(&_kernel_pg_dir), _PG_RW_FLAG);
}
