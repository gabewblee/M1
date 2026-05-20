#include "bits.h"
#include "boot/setup.h"
#include "config.h"
#include "kernel/panic.h"
#include "lib/string.h"
#include "mm/page.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

#define PG_DIR_SELF_IDX         1023u                                                                /* Recursive page directory index                   */
#define PG_DIR_SELF_VA          ((virt_addr_t)((PG_DIR_SELF_IDX << 22u) | (PG_DIR_SELF_IDX << 12u))) /* Page directory virtual address                   */
#define PG_TABLE_VA(pg_dir_idx) ((virt_addr_t)((PG_DIR_SELF_IDX << 22u) | ((pg_dir_idx) << 12u)))    /* Page table virtual address                       */
#define PG_DIR_HIGHER_HALF_IDX  ((u32)(HIGHER_HALF_OFFSET >> 22u))                                   /* Page directory higher half index                 */
#define PG_DIR_SPAN             (1u << 22u)                                                          /* Virtual span covered by one page directory entry */
#define SCRATCH_VA              0xFEC00000u                                                          /* Scratch page virtual address                     */

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
extern u8  skheap[];
extern u8  ekheap[];

static inline u32 get_pg_dir_idx(virt_addr_t vaddr) {
    return vaddr >> 22;
}

static inline u32 get_pg_table_idx(virt_addr_t vaddr) {
    return (vaddr >> 12) & 0x3FFu;
}

static inline pg_dir_t* get_cur_pg_dir(void) {
    return (pg_dir_t*)PG_DIR_SELF_VA;
}

static inline pg_table_t* get_cur_pg_table(u32 pg_dir_idx) {
    return (pg_table_t*)PG_TABLE_VA(pg_dir_idx);
}

static inline pg_dir_entry_t* get_pg_dir_entry(u32 pg_dir_idx) {
    return &get_cur_pg_dir()->entries[pg_dir_idx];
}

static inline pg_table_entry_t* get_pg_table_entry(u32 pg_dir_idx, u32 pg_table_idx) {
    return &get_cur_pg_table(pg_dir_idx)->entries[pg_table_idx];
}

static inline void set_pg_dir_entry(pg_dir_entry_t* entry, phys_addr_t paddr, u32 flags) {
    entry->present = 1;
    entry->rw      = 1;
    entry->user    = (flags & PG_FLAG_USER) ? 1 : 0;
    entry->paddr   = paddr >> 12;
}

static inline void set_pg_table_entry(pg_table_entry_t* entry, phys_addr_t paddr, u32 flags) {
    entry->present = 1;
    entry->rw      = (flags & PG_FLAG_RW) ? 1 : 0;
    entry->user    = (flags & PG_FLAG_USER) ? 1 : 0;
    entry->global  = (flags & PG_FLAG_GLOBAL) ? 1 : 0;
    entry->paddr   = paddr >> 12;
}

static inline void flush_tlb_mapping(virt_addr_t vaddr) {
    __asm__ volatile ("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static inline void flush_tlb(void) {
    u32 cr3;
    __asm__ volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=r"(cr3) :: "memory");
}

static void __init alloc_pg_table(virt_addr_t vaddr) {
    u32 pg_dir_idx = get_pg_dir_idx(vaddr);
    pg_dir_entry_t* pg_dir_entry = get_pg_dir_entry(pg_dir_idx);
    if (pg_dir_entry->present)
        return;

    phys_addr_t paddr = pmm_alloc_frm();
    if (unlikely(!paddr))
        PANIC("Error: Failed to allocate frame");

    set_pg_dir_entry(pg_dir_entry, paddr, PG_FLAG_RW);
    flush_tlb_mapping(PG_TABLE_VA(pg_dir_idx));
    memset(get_cur_pg_table(pg_dir_idx), 0, sizeof(pg_table_t));
}

void vmm_set_pg_flags(virt_addr_t vaddr, u32 flags) {
    u32 pg_dir_idx = get_pg_dir_idx(vaddr);
    pg_dir_entry_t* pg_dir_entry = get_pg_dir_entry(pg_dir_idx);
    if (unlikely(!pg_dir_entry->present))
        return;

    u32 pg_table_idx = get_pg_table_idx(vaddr);
    pg_table_entry_t* pg_table_entry = get_pg_table_entry(pg_dir_idx, pg_table_idx);
    if (unlikely(!pg_table_entry->present))
        return;

    pg_table_entry->rw     = (flags & PG_FLAG_RW) ? 1 : 0;
    pg_table_entry->user   = (flags & PG_FLAG_USER) ? 1 : 0;
    pg_table_entry->global = (flags & PG_FLAG_GLOBAL) ? 1 : 0;
    flush_tlb_mapping(vaddr);
}

phys_addr_t vmm_create_aspace(void) {
    phys_addr_t paddr = pmm_alloc_frm();
    if (unlikely(!paddr))
        return 0;

    vmm_map_pg(paddr, SCRATCH_VA, PG_FLAG_RW);
    pg_dir_t* new_pg_dir = (pg_dir_t*)SCRATCH_VA;
    memset(new_pg_dir, 0, sizeof(pg_dir_t));

    const pg_dir_t* master_pg_dir = (const pg_dir_t*)swapper_pg_dir;
    for (u32 pg_dir_idx = PG_DIR_HIGHER_HALF_IDX; pg_dir_idx < PG_DIR_SELF_IDX; pg_dir_idx++)
        new_pg_dir->entries[pg_dir_idx] = master_pg_dir->entries[pg_dir_idx];

    set_pg_dir_entry(&new_pg_dir->entries[PG_DIR_SELF_IDX], paddr, PG_FLAG_RW);
    vmm_unmap_pg(SCRATCH_VA);
    return paddr;
}

void vmm_destroy_aspace(phys_addr_t cr3) {
    if (unlikely(!cr3))
        return;

    vmm_map_pg(cr3, SCRATCH_VA, PG_FLAG_RW);
    pg_dir_t* pg_dir = (pg_dir_t*)SCRATCH_VA;
    for (u32 pg_dir_idx = 0; pg_dir_idx < PG_DIR_HIGHER_HALF_IDX; pg_dir_idx++) {
        if (!pg_dir->entries[pg_dir_idx].present)
            continue;

        phys_addr_t paddr = (phys_addr_t)pg_dir->entries[pg_dir_idx].paddr << 12;
        vmm_unmap_pg(SCRATCH_VA);
        vmm_map_pg(paddr, SCRATCH_VA, PG_FLAG_RW);

        pg_table_t* pg_table = (pg_table_t*)SCRATCH_VA;
        for (u32 pg_table_idx = 0; pg_table_idx < 1024; pg_table_idx++) {
            if (!pg_table->entries[pg_table_idx].present)
                continue;

            pmm_free_frm((phys_addr_t)pg_table->entries[pg_table_idx].paddr << 12);
        }

        vmm_unmap_pg(SCRATCH_VA);
        pmm_free_frm(paddr);
        vmm_map_pg(cr3, SCRATCH_VA, PG_FLAG_RW);
        pg_dir = (pg_dir_t*)SCRATCH_VA;
    }

    vmm_unmap_pg(SCRATCH_VA);
    pmm_free_frm(cr3);
}

void vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags) {
    u32 pg_dir_idx = get_pg_dir_idx(vaddr);
    pg_dir_entry_t* pg_dir_entry = get_pg_dir_entry(pg_dir_idx);
    if (!pg_dir_entry->present) {
        phys_addr_t paddr = pmm_alloc_frm();
        if (unlikely(!paddr))
            PANIC("Error: Failed to allocate frame");

        set_pg_dir_entry(pg_dir_entry, paddr, flags);
        flush_tlb_mapping(PG_TABLE_VA(pg_dir_idx));
        memset(get_cur_pg_table(pg_dir_idx), 0, sizeof(pg_table_t));
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

void vmm_demand_map(virt_addr_t vaddr, u32 flags) {
    phys_addr_t paddr = pmm_alloc_frm();
    if (unlikely(!paddr))
        PANIC("Error: Failed to allocate frame");

    vmm_map_pg(paddr, vaddr, flags);
}

void __init vmm_init(void) {
    pg_dir_t* master_pg_dir = (pg_dir_t*)swapper_pg_dir;
    set_pg_dir_entry(&master_pg_dir->entries[PG_DIR_SELF_IDX], __pa(swapper_pg_dir), PG_FLAG_RW);
    flush_tlb();

    virt_addr_t start = ALIGN_DOWN_TO((virt_addr_t)skheap, PG_DIR_SPAN);
    virt_addr_t end = ALIGN_UP_TO((virt_addr_t)ekheap, PG_DIR_SPAN);
    for (virt_addr_t vaddr = start; vaddr < end; vaddr += PG_DIR_SPAN)
        alloc_pg_table(vaddr);

    alloc_pg_table(SCRATCH_VA);
}
