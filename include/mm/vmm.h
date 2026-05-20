#pragma once

#include "config.h"

#define PG_FLAG_RW     0x002u /* Page is writable        */
#define PG_FLAG_USER   0x004u /* Page is user-accessible */
#define PG_FLAG_GLOBAL 0x100u /* Page is global          */

/**
 * vmm_set_pg_flags - Updates page flags for an existing mapping.
 * @vaddr: The virtual address whose mapping flags to update.
 * @flags: The new page table flags.
 */
void vmm_set_pg_flags(virt_addr_t vaddr, u32 flags);

/**
 * vmm_create_aspace - Creates a new address space. Higher-half mappings are
 *                     shared with the kernel page directory.
 * @return: The physical address of the new page directory, or 0 on failure.
 */
phys_addr_t vmm_create_aspace(void);

/**
 * vmm_destroy_aspace - Destroys an address space. Frees lower-half page tables,
 *                      lower-half mapped frames, and page directory.
 * @cr3: The physical address of the page directory to destroy.
 */
void vmm_destroy_aspace(phys_addr_t cr3);

/**
 * vmm_map_pg - Maps @vaddr to @paddr with the given flags.
 * @paddr: The physical address to map to.
 * @vaddr: The virtual address to map.
 * @flags: The flags for the page table entry.
 */
void vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * vmm_unmap_pg - Unmaps the page at @vaddr. No-op if @vaddr is not 
 *                currently mapped.
 * @vaddr: The virtual address to unmap.
 */
void vmm_unmap_pg(virt_addr_t vaddr);

/**
 * vmm_demand_map - Allocates a physical frame and maps it at the 
 *                  page-aligned address @vaddr.
 * @vaddr: The virtual address to map.
 * @flags: The flags for the page table entry.
 */
void vmm_demand_map(virt_addr_t vaddr, u32 flags);

/**
 * vmm_init - Initializes the virtual memory manager. Maps the kernel 
 *            page directory.
 */
void __init vmm_init(void);