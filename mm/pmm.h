#pragma once

#include "../boot/multiboot.h"
#include "../config.h"

/**
 * pmm_free_init_section - Free the init section
 */
void pmm_free_init_section(void);

/**
 * pmm_alloc_frm - Allocate a memory frame
 * @return: The allocated memory frame
 */
phys_addr_t __hot pmm_alloc_frm(void);

/**
 * pmm_free_frm - Free a memory frame
 * @pg: The memory frame to free
 */
void __hot pmm_free_frm(phys_addr_t pg);

/**
 * pmm_init - Initialize the PMM
 * @mbinfo: The multiboot information structure
 */
void __init pmm_init(multiboot_info_t* mbinfo);