#pragma once

#include <boot/multiboot.h>
#include <config.h>

/**
 * pmm_free_init_section - Frees the frames occupied by the .init section.
 * 
 * Context: The .init section contains code that is only needed during
 *          initialization. After the kernel is fully operational, these
 *          frames can be freed.
 */
void pmm_free_init_section(void);

/**
 * pmm_alloc_frm - Allocates a page-aligned physical frame.
 * Returns: The physical address of the allocated frame on success, or 0 on failure.
 */
phys_addr_t pmm_alloc_frm(void);

/**
 * pmm_free_frm - Frees the page-aligned physical frame found at @paddr.
 * @paddr: The physical address of the frame to free.
 */
void pmm_free_frm(phys_addr_t paddr);

/**
 * pmm_alloc_frms - Allocates @cnt physically contiguous, page-aligned
 *                  physical frames.
 * @cnt: The number of contiguous frames to allocate.
 * Returns: The physical address of the first allocated frame on success, or 0 on failure.
 */
phys_addr_t pmm_alloc_frms(u32 cnt);

/**
 * pmm_free_frms - Frees @cnt physically contiguous, page-aligned physical 
 *                 frames starting at @paddr.
 * @paddr: The physical address of the first frame to free.
 * @cnt: The number of frames to free.
 */
void pmm_free_frms(phys_addr_t paddr, u32 cnt);
