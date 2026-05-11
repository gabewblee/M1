#include <stdbool.h>

#include "pmm.h"
#include "vmm.h"

#include "../config.h"
#include "../kernel/panic.h"
#include "../lib/string.h"

#define VMM_PG_RW_FLAG     0x02     /* Read/write flag */
#define VMM_PG_USER_FLAG   0x04     /* User/supervisor flag */
#define VMM_PG_GLOBAL_FLAG 0x100    /* Global page flag */

typedef struct pg_dir_entry_t {
    u8          present   : 1;      /* Present */
    u8          rw        : 1;      /* Read/Write */
    u8          user      : 1;      /* User/Supervisor */
    u8          pwt       : 1;      /* Page-level write-through */
    u8          pcd       : 1;      /* Page-level cache disable */
    u8          accessed  : 1;      /* Accessed */
    u8          lower_avl : 1;      /* Available for system programmer's use */
    u8          ps        : 1;      /* Page size */
    u8          upper_avl : 4;      /* Available for system programmer's use */
    phys_addr_t paddr     : 20;     /* Physical address of the page table */
} __packed pg_dir_entry_t;

typedef struct pg_table_entry_t {
    u8          present  : 1;       /* Present */
    u8          rw       : 1;       /* Read/Write */
    u8          user     : 1;       /* User/Supervisor */
    u8          pwt      : 1;       /* Page-level write-through */
    u8          pcd      : 1;       /* Page-level cache disable */
    u8          accessed : 1;       /* Accessed */
    u8          dirty    : 1;       /* Dirty */
    u8          pat      : 1;       /* Page attribute table */
    u8          global   : 1;       /* Global */
    u8          avl      : 3;       /* Available for system programmer's use */
    phys_addr_t paddr    : 20;      /* Physical address of the page */
} __packed pg_table_entry_t;

typedef struct pg_dir_t {
    pg_dir_entry_t entries[1024];   /* Page directory entries */
} __aligned(PG_SZ) pg_dir_t;

typedef struct pg_table_t {
    pg_table_entry_t entries[1024]; /* Page table entries */
} __aligned(PG_SZ) pg_table_t;

__aligned(PG_SZ)
static pg_dir_t kernel_pg_dir;      /* Kernel page directory */
extern u8       _skernel[];         /* Start of the kernel in memory */
extern u8       _ekernel[];         /* End of the kernel in memory */
extern u8       _skheap[];          /* Start of the kernel heap in memory */
extern u8       _ekheap[];          /* End of the kernel heap in memory */
extern u8       _skstack[];         /* Start of the kernel stack in memory */
extern u8       _ekstack[];         /* End of the kernel stack in memory */

static inline u32 vmm_get_pg_dir_idx(virt_addr_t vaddr) {
    return vaddr >> 22;
}

static inline void vmm_set_pg_dir_entry(pg_dir_entry_t* entry, phys_addr_t paddr, u32 flags) {
    entry->present = 1;
    entry->rw      = 1;
    entry->user    = (flags & VMM_PG_USER_FLAG) ? 1 : 0;
    entry->paddr   = __pg_shr(paddr);
}

static inline u32 vmm_get_pg_table_idx(virt_addr_t vaddr) {
    return (vaddr >> 12) & 0x3FF;
}

static inline pg_table_t* vmm_get_pg_table(u32 pg_dir_idx) {
    phys_addr_t paddr = __pg_shl(kernel_pg_dir.entries[pg_dir_idx].paddr);
    return (pg_table_t*)__va(paddr);
}

static inline void vmm_set_pg_table_entry(pg_table_entry_t* entry, phys_addr_t paddr, u32 flags) {
    entry->present = 1;
    entry->rw      = (flags & VMM_PG_RW_FLAG) ? 1 : 0;
    entry->user    = (flags & VMM_PG_USER_FLAG) ? 1 : 0;
    entry->global  = (flags & VMM_PG_GLOBAL_FLAG) ? 1 : 0;
    entry->paddr   = __pg_shr(paddr);
}

static inline void vmm_map_range(phys_addr_t start, phys_addr_t end, u32 flags) {
    for (phys_addr_t paddr = start; paddr < end; paddr += PG_SZ)
        vmm_map_pg(paddr, __va(paddr), flags);
}

static inline void vmm_map_kernel_pg_dir(void) {
    vmm_map_pg(__pa(&kernel_pg_dir), (virt_addr_t)&kernel_pg_dir, VMM_PG_RW_FLAG);
}

static inline void vmm_map_vga(void) {
    vmm_map_pg(VGA_PHYS_ADDR, __va(VGA_PHYS_ADDR), VMM_PG_RW_FLAG);
}

static inline void vmm_map_kernel(void) {
    vmm_map_range(__pa(_skernel), __pa(_ekernel), VMM_PG_RW_FLAG);
}

static inline void vmm_map_kheap(void) {
    vmm_map_range(__pa(_skheap), __pa(_ekheap), VMM_PG_RW_FLAG);
}

static inline void vmm_kernel_pg_dir_init(void) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(__pa(&kernel_pg_dir)) : "memory");
}

static inline void vmm_flush_tlb(void) {
    __asm__ volatile("mov %%cr3, %%eax\n mov %%eax, %%cr3" : : : "eax", "memory");
}

static inline void vmm_invalidate_tlb_mapping(virt_addr_t vaddr) {
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

u32 vmm_get_cr3(void) {
    u32 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void __hot vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags) {
    u32 pg_dir_idx = vmm_get_pg_dir_idx(vaddr);
    if (!kernel_pg_dir.entries[pg_dir_idx].present) {
        phys_addr_t frm = pmm_alloc_frm();
        if (!frm)
            PANIC("Error: Failed to allocate page frame");

        pg_table_t* pg_table = (pg_table_t*)__va(frm);
        memset(pg_table, 0, sizeof(pg_table_t));
        vmm_set_pg_dir_entry(&kernel_pg_dir.entries[pg_dir_idx], frm, flags);
    }

    u32 pg_table_idx = vmm_get_pg_table_idx(vaddr);
    pg_table_t* pg_table = vmm_get_pg_table(pg_dir_idx);
    vmm_set_pg_table_entry(&pg_table->entries[pg_table_idx], paddr, flags);
    vmm_invalidate_tlb_mapping(vaddr);
}

void __hot vmm_unmap_pg(virt_addr_t vaddr) {
    u32 pg_dir_idx = vmm_get_pg_dir_idx(vaddr);
    if (!kernel_pg_dir.entries[pg_dir_idx].present)
        return;
    
    u32 pg_table_idx = vmm_get_pg_table_idx(vaddr);
    pg_table_t* pg_table = vmm_get_pg_table(pg_dir_idx);
    pg_table->entries[pg_table_idx].present = 0;
    vmm_invalidate_tlb_mapping(vaddr);
}

void __init vmm_init(void) {
    /* Map kernel page directory */
    vmm_map_kernel_pg_dir();

    /* Map VGA address space */
    vmm_map_vga();

    /* Map kernel address space */
    vmm_map_kernel();

    /* Map kernel heap address space */
    vmm_map_kheap();

    /* Switch to kernel page directory */
    vmm_kernel_pg_dir_init();

    /* Flush the TLB */
    vmm_flush_tlb();
}