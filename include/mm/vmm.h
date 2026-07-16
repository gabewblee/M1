#pragma once

#include <arch/x86/idt.h>
#include <config.h>
#include <kernel/sync/spinlock.h>
#include <mm.h>
#include <stdbool.h>
#include <stddef.h>

extern spinlock_s vmm_scratch_lk;

#define VMM_SCRATCH_BASE 0xFEC00000u                /* Scratch page table virtual address */
#define VMM_MMU_SCRATCH  VMM_SCRATCH_BASE           /* MMU scratch frame virtual address  */
#define VMM_IPC_SCRATCH  (VMM_SCRATCH_BASE + PG_SZ) /* IPC scratch frame virtual address  */

/**
 * vmm_pg_fault_handler - Handles a page-fault exception.
 * @frm: The pointer to the interrupt stack frame.
 */
void vmm_pg_fault_handler(ifrm_s* frm);

/**
 * vmm_set_pg_flags - Updates the mapping at @vaddr with @flags.
 * @vaddr: The virtual address of the mapping to update.
 * @flags: The flags to set.
 */
void vmm_set_pg_flags(virt_addr_t vaddr, u32 flags);

/**
 * vmm_range_accessible - Checks that every page in [@vaddr, @vaddr + @nbytes)
 *                        is mapped with @flags in the active address space.
 * @vaddr: The start virtual address of the range.
 * @nbytes: The byte length of the range.
 * @flags: The required page flags.
 * Returns: true if satisfied, false otherwise.
 */
bool vmm_range_accessible(virt_addr_t vaddr, size_t nbytes, u32 flags);

/**
 * vmm_aspace_init - Initializes @pg_dir, shares higher-half mappings, and installs
 *                   recursive self-mapping.
 * @pg_dir: The page directory virtual address.
 */
void vmm_aspace_init(void* pg_dir);

/**
 * vmm_create_aspace - Creates a new address space. Higher-half mappings are
 *                     shared with the kernel page directory.
 * Returns: The physical address of the new page directory on success, or 0 on failure.
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
 * @flags: The page table entry flags.
 */
void vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * vmm_map_pg_in - Maps @vaddr to @paddr with @flags in the address space @cr3.
 * @cr3: The page directory physical address.
 * @paddr: The physical address to map to.
 * @vaddr: The virtual address to map.
 * @flags: The page table entry flags.
 */
void vmm_map_pg_in(phys_addr_t cr3, phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * vmm_map_pg_table_in - Maps the page table at @pg_table_paddr into the page directory @cr3, 
 *                       covering the region containing @vaddr.
 * @cr3: The page directory physical address.
 * @pg_table_paddr: The page table physical address.
 * @vaddr: The virtual address to cover.
 * @flags: The page directory entry flags.
 * Returns: E_OK on success, or -E_EXIST on failure.
 */
i32 vmm_map_pg_table_in(phys_addr_t cr3, phys_addr_t pg_table_paddr, virt_addr_t vaddr, u32 flags);

/**
 * vmm_map_frm_in - Maps @vaddr to @paddr in the address space @cr3. The table covering @vaddr
 *                  must already be mapped.
 * @cr3: The page directory physical address.
 * @paddr: The physical address to map to.
 * @vaddr: The virtual address to map.
 * @flags: The page table entry flags.
 * Returns: E_OK on success, or -E_FAULT on failure.
 */
i32 vmm_map_frm_in(phys_addr_t cr3, phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * vmm_unmap_pg - Unmaps the mapping at @vaddr. No-op if @vaddr is not mapped.
 * @vaddr: The virtual address to unmap.
 */
void vmm_unmap_pg(virt_addr_t vaddr);

/**
 * vmm_unmap_pg_in - Unmaps the mapping at @vaddr in the address space @cr3. No-op if
 *                   @vaddr is not mapped.
 * @cr3: The page directory physical address.
 * @vaddr: The virtual address to unmap.
 */
void vmm_unmap_pg_in(phys_addr_t cr3, virt_addr_t vaddr);

/**
 * vmm_demand_map - Allocates a physical frame and maps it at @vaddr with @flags.
 * @vaddr: The virtual address to map.
 * @flags: The flags to set.
 */
void vmm_demand_map(virt_addr_t vaddr, u32 flags);
