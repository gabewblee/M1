#pragma once

#include "config.h"
#include "arch/x86/idt.h"
#include "uapi/mm.h"

#define VMM_SCRATCH_BASE 0xFEC00000u                /* Scratch page table virtual address */
#define VMM_MMU_SCRATCH  VMM_SCRATCH_BASE           /* MMU scratch frame virtual address  */
#define VMM_IPC_SCRATCH  (VMM_SCRATCH_BASE + PG_SZ) /* IPC scratch frame virtual address  */

/**
 * vmm_pg_fault_handler - Handles a page-fault exception.
 * @frm: The pointer to the interrupt stack frame.
 */
void vmm_pg_fault_handler(const int_frm_s* frm);

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
 * vmm_destroy_aspace - Destroys the address space @cr3.
 * @cr3: The physical address of the page directory to destroy.
 * 
 * Preconditions:
 * - @cr3 must not be active.
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
 * vmm_init - Initializes the virtual memory manager (VMM).
 */
void __init vmm_init(void);