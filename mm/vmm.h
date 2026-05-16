#pragma once

#include "../boot/multiboot.h"
#include "../config.h"

/**
 * vmm_map_pg - Maps @vaddr to @paddr with the given flags.
 * @paddr: The physical address to map to.
 * @vaddr: The virtual address to map.
 * @flags: The flags for the page table entry.
 */
void __hot vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * vmm_unmap_pg - Unmaps the page at @vaddr. No-op if @vaddr is not 
 *                currently mapped.
 * @vaddr: The virtual address to unmap.
 */
void __hot vmm_unmap_pg(virt_addr_t vaddr);

/**
 * vmm_demand_map - Allocates a physical frame and maps it at the 
 *                  page-aligned address @vaddr.
 * @vaddr: The virtual address to map.
 */
void __hot vmm_demand_map(virt_addr_t vaddr);

/**
 * vmm_init - Initializes the virtual memory manager. Maps the kernel 
 *            page directory.
 */
void __init vmm_init(void);