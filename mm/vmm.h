#pragma once

#include "../boot/multiboot.h"
#include "../config.h"

/**
 * vmm_get_cr3 - Get the current value of the CR3 register
 * @return: The current value of the CR3 register
 */
u32 vmm_get_cr3(void);

/**
 * vmm_map_pg - Map a page at the given virtual address to the given physical address with the given flags
 * @paddr: The physical address to map to
 * @vaddr: The virtual address being mapped
 * @flags: The flags for the page
 */
void __hot vmm_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags);

/**
 * vmm_unmap_pg - Unmap a page at the given virtual address
 * @vaddr: The virtual address to unmap
 */
void __hot vmm_unmap_pg(virt_addr_t vaddr);

/**
 * vmm_init - Initialize the VMM
 */
void __init vmm_init(void);