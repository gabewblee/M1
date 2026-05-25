#include <stddef.h>

#include "arch/x86/idt.h"
#include "bits.h"
#include "boot/setup.h"
#include "config.h"
#include "kernel/panic.h"
#include "lib/string.h"
#include "mm/page.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

#define PG_DIR_IDX           1023u                                                        /* Page directory index             */
#define PG_DIR               ((virt_addr_t)((PG_DIR_IDX << 22u) | (PG_DIR_IDX << 12u)))   /* Page directory virtual address   */
#define PG_TABLE(pg_dir_idx) ((virt_addr_t)((PG_DIR_IDX << 22u) | ((pg_dir_idx) << 12u))) /* Page table virtual address       */
#define HIGHER_HALF_IDX      ((u32)(HIGHER_HALF_OFFSET >> 22u))                           /* Page directory higher half index */

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

extern u32       swapper_pg_dir[1024];
extern const u8  skheap[];
extern const u8  ekheap[];

static inline u32 get_pg_dir_idx(virt_addr_t vaddr) {
    return vaddr >> 22;
}

static inline u32 get_pg_table_idx(virt_addr_t vaddr) {
    return (vaddr >> 12) & 0x3FFu;
}

static inline pg_dir_t* get_pg_dir(void) {
    return (pg_dir_t*)PG_DIR;
}

static inline pg_table_t* get_pg_table(u32 pg_dir_idx) {
    return (pg_table_t*)PG_TABLE(pg_dir_idx);
}

static inline pg_dir_entry_t* get_pg_dir_entry(u32 pg_dir_idx) {
    return &get_pg_dir()->entries[pg_dir_idx];
}

static inline pg_table_entry_t* get_pg_table_entry(u32 pg_dir_idx, u32 pg_table_idx) {
    return &get_pg_table(pg_dir_idx)->entries[pg_table_idx];
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
    flush_tlb_mapping(PG_TABLE(pg_dir_idx));
    memset(get_pg_table(pg_dir_idx), 0, sizeof(pg_table_t));
}

void vmm_pg_fault_handler(const int_frm_t* frm) {
    if (likely(!(frm->err_code & 1))) {
        virt_addr_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        if (likely(cr2 >= (virt_addr_t)skheap && cr2 < (virt_addr_t)ekheap)) {
            vmm_demand_map(cr2 & ~(PG_SZ - 1u), PG_FLAG_RW);
            return;
        }
    }

    PANIC("Error: CPU exception thrown");
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

    vmm_map_pg(paddr, VMM_MMU_SCRATCH, PG_FLAG_RW);
    pg_dir_t* new = (pg_dir_t*)VMM_MMU_SCRATCH;
    memset(new, 0, sizeof(pg_dir_t));

    const pg_dir_t* kernel_pg_dir = (const pg_dir_t*)swapper_pg_dir;
    for (u32 pg_dir_idx = HIGHER_HALF_IDX; pg_dir_idx < PG_DIR_IDX; pg_dir_idx++)
        new->entries[pg_dir_idx] = kernel_pg_dir->entries[pg_dir_idx];

    set_pg_dir_entry(&new->entries[PG_DIR_IDX], paddr, PG_FLAG_RW);
    vmm_unmap_pg(VMM_MMU_SCRATCH);
    return paddr;
}

void vmm_destroy_aspace(phys_addr_t cr3) {
    if (unlikely(!cr3))
        return;

    vmm_map_pg(cr3, VMM_MMU_SCRATCH, PG_FLAG_RW);
    pg_dir_t* kernel_pg_dir = (pg_dir_t*)VMM_MMU_SCRATCH;
    for (u32 pg_dir_idx = 0; pg_dir_idx < HIGHER_HALF_IDX; pg_dir_idx++) {
        if (!kernel_pg_dir->entries[pg_dir_idx].present)
            continue;

        phys_addr_t paddr = (phys_addr_t)kernel_pg_dir->entries[pg_dir_idx].paddr << 12;
        vmm_map_pg(paddr, VMM_MMU_SCRATCH, PG_FLAG_RW);

        pg_table_t* pg_table = (pg_table_t*)VMM_MMU_SCRATCH;
        for (u32 pg_table_idx = 0; pg_table_idx < 1024; pg_table_idx++) {
            if (!pg_table->entries[pg_table_idx].present)
                continue;

            pmm_free_frm((phys_addr_t)pg_table->entries[pg_table_idx].paddr << 12);
        }

        pmm_free_frm(paddr);
        vmm_map_pg(cr3, VMM_MMU_SCRATCH, PG_FLAG_RW);
        kernel_pg_dir = (pg_dir_t*)VMM_MMU_SCRATCH;
    }

    vmm_unmap_pg(VMM_MMU_SCRATCH);
    pmm_free_frm(cr3);
}

void vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags) {
    u32 pg_dir_idx = get_pg_dir_idx(vaddr);
    pg_dir_entry_t* pg_dir_entry = get_pg_dir_entry(pg_dir_idx);
    if (!pg_dir_entry->present) {
        phys_addr_t frm = pmm_alloc_frm();
        if (unlikely(!frm))
            PANIC("Error: Failed to allocate frame");

        set_pg_dir_entry(pg_dir_entry, frm, flags);
        flush_tlb_mapping(PG_TABLE(pg_dir_idx));
        memset(get_pg_table(pg_dir_idx), 0, sizeof(pg_table_t));
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
    set_pg_dir_entry(&((pg_dir_t*)swapper_pg_dir)->entries[PG_DIR_IDX], __pa(swapper_pg_dir), PG_FLAG_RW);
    flush_tlb();
    
    size_t span = (1u << 22u);
    virt_addr_t start = ALIGN_DOWN_TO((virt_addr_t)skheap, span), end = ALIGN_UP_TO((virt_addr_t)ekheap, span);
    for (virt_addr_t vaddr = start; vaddr < end; vaddr += span)
        alloc_pg_table(vaddr);

    alloc_pg_table(VMM_SCRATCH_BASE);

    phys_addr_t ipc_scratch_frm = pmm_alloc_frm();
    if (unlikely(!ipc_scratch_frm))
        PANIC("Error: Failed to allocate frame");

    vmm_map_pg(ipc_scratch_frm, VMM_IPC_SCRATCH, PG_FLAG_RW | PG_FLAG_GLOBAL);

    /* Register page fault handler */
    idt_register_handler(14, vmm_pg_fault_handler);
}
