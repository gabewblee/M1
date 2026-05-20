#include <stdbool.h>

#include "boot/setup.h"
#include "config.h"
#include "kernel/panic.h"
#include "lib/string.h"
#include "mm/page.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

#define PG_FLAG_RW             0x02                                     /* Read/write flag                               */
#define PG_FLAG_USER           0x04                                     /* User/supervisor flag                          */
#define PG_FLAG_GLOBAL         0x100                                    /* Global flag                                   */
#define PG_DIR_SELF_IDX        1023u                                    /* Recursive page directory index                */
#define PG_TABLE(pg_table_idx)       \
    ((virt_addr_t)((PG_DIR_SELF_IDX << 22u) | ((pg_table_idx) << 12u))) /* Returns the virtual address of the page table */
#define KERNEL_PG_DIR          (*(pg_dir_t*)swapper_pg_dir)             /* Swapper page directory alias                  */

typedef struct pg_dir_entry_t {
    u8          present   : 1;  /* Present flag                */
    u8          rw        : 1;  /* Writable flag               */
    u8          user      : 1;  /* User access flag            */
    u8          pwt       : 1;  /* Write through flag          */
    u8          pcd       : 1;  /* Cache disable flag          */
    u8          accessed  : 1;  /* Accessed flag               */
    u8          lower_avl : 1;  /* Available for use           */
    u8          ps        : 1;  /* Page size flag              */
    u8          upper_avl : 4;  /* Available for use           */
    phys_addr_t paddr     : 20; /* Physical page frame address */
} __packed pg_dir_entry_t;

typedef struct pg_table_entry_t {
    u8          present  : 1;  /* Present flag                */
    u8          rw       : 1;  /* Writable flag               */
    u8          user     : 1;  /* User access flag            */
    u8          pwt      : 1;  /* Write through flag          */
    u8          pcd      : 1;  /* Cache disable flag          */
    u8          accessed : 1;  /* Accessed flag               */
    u8          dirty    : 1;  /* Dirty flag                  */
    u8          pat      : 1;  /* Page attribute table flag   */
    u8          global   : 1;  /* Global mapping flag         */
    u8          avl      : 3;  /* Available for use           */
    phys_addr_t paddr    : 20; /* Physical page frame address */
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
    return &KERNEL_PG_DIR.entries[pg_dir_idx];
}

static inline void set_pg_dir_entry(pg_dir_entry_t* entry, phys_addr_t paddr, u32 flags) {
    entry->present = 1;
    entry->rw      = 1;
    entry->user    = (flags & PG_FLAG_USER) ? 1 : 0;
    entry->paddr   = paddr >> 12;
}

static inline u32 get_pg_table_idx(virt_addr_t vaddr) {
    return (vaddr >> 12) & 0x3FF;
}

static inline pg_table_entry_t* get_pg_table_entry(u32 pg_dir_idx, u32 pg_table_idx) {
    pg_table_t* pg_table = (pg_table_t*)PG_TABLE(pg_dir_idx);
    return &pg_table->entries[pg_table_idx];
}

static inline void set_pg_table_entry(pg_table_entry_t* entry, phys_addr_t paddr, u32 flags) {
    entry->present = 1;
    entry->rw      = (flags & PG_FLAG_RW) ? 1 : 0;
    entry->user    = (flags & PG_FLAG_USER) ? 1 : 0;
    entry->global  = (flags & PG_FLAG_GLOBAL) ? 1 : 0;
    entry->paddr   = paddr >> 12;
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
        memset((pg_table_t*)PG_TABLE(pg_dir_idx), 0, sizeof(pg_table_t));
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

    vmm_map_pg(paddr, vaddr, PG_FLAG_RW);
}

void __init vmm_init(void) {
    pg_dir_entry_t* pg_dir_entry = get_pg_dir_entry(PG_DIR_SELF_IDX);
    set_pg_dir_entry(pg_dir_entry, __pa(&KERNEL_PG_DIR), PG_FLAG_RW);
}
