#pragma once

#include "config.h"
#include "uapi.h"

typedef struct int_frm_t int_frm_t;

/**
 * vmm_pg_fault_handler - Handles page fault exceptions.
 * @frm: The pointer to the interrupt stack frame.
 *
 * Supports:
 * - Kernel heap demand mapping
 * - CPU exceptions
 */
void vmm_pg_fault_handler(const int_frm_t* frm);

/**
 * vmm_set_pg_flags - Updates the mapping at @vaddr with @flags.
 * @vaddr: The virtual address of the mapping to update.
 * @flags: The flags to set.
 */
void vmm_set_pg_flags(virt_addr_t vaddr, u32 flags);

/**
 * vmm_create_aspace - Creates a new address space. Higher-half mappings are
 *                     shared with the kernel page directory.
 * Returns: The physical address of the new page directory, or 0 on failure.
 */
phys_addr_t vmm_create_aspace(void);

/**
 * vmm_destroy_aspace - Destroys an address space. Frees lower-half page tables,
 *                      lower-half mapped frames, and page directory. Must not
 *                      be called on an active CR3.
 * @cr3: The physical address of the page directory to destroy.
 */
void vmm_destroy_aspace(phys_addr_t cr3);

/**
 * vmm_map_pg - Maps @vaddr to @paddr with @flags.
 * @paddr: The physical address to map to.
 * @vaddr: The virtual address to map.
 * @flags: The flags for the page table entry.
 */
void vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * vmm_unmap_pg - Unmaps the mapping at @vaddr. No-op if @vaddr is not mapped.
 * @vaddr: The virtual address to unmap.
 */
void vmm_unmap_pg(virt_addr_t vaddr);

/**
 * vmm_demand_map - Allocates a physical frame and maps it at @vaddr with @flags.
 * @vaddr: The virtual address to map.
 * @flags: The flags to set.
 */
void vmm_demand_map(virt_addr_t vaddr, u32 flags);

/**
 * vmm_init - Initializes the virtual memory manager. Maps the kernel 
 *            page directory. Pre-allocates page tables for the kernel heap
 *            and the scratch page. Registers the page fault handler.
 *
 * Context: The kernel heap page tables are pre-allocated to provide a consistent
 *          address space, since the kernel heap is shared among all tasks.
 */
void __init vmm_init(void);