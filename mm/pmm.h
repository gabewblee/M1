#pragma once

#include "../boot/multiboot.h"
#include "../config.h"

/**
 * pmm_free_init_section - Frees the frames occupied by the .init section.
 */
void pmm_free_init_section(void);

/**
 * pmm_alloc_frm - Allocates a page-aligned physical frame.
 * @return: The physical address of the allocated frame, or 0 on failure.
 */
phys_addr_t pmm_alloc_frm(void);

/**
 * pmm_free_frm - Frees the physical frame found at @paddr. No-op if 
 *                @paddr is 0.
 * @paddr: The physical address of the frame to free.
 */
void pmm_free_frm(phys_addr_t paddr);

/**
 * pmm_alloc_frms - Allocates @cnt physically contiguous, page-aligned
 *                  frames.
 * @cnt: The number of contiguous frames to allocate.
 * @return: The physical address of the first frame, or 0 on failure.
 */
phys_addr_t pmm_alloc_frms(u32 cnt);

/**
 * pmm_free_frms - Frees @cnt physically contiguous frames starting at 
 *                 @paddr. No-op if @paddr or @cnt is 0.
 * @paddr: The physical address of the first frame to free.
 * @cnt: The number of frames to free.
 */
void pmm_free_frms(phys_addr_t paddr, u32 cnt);

/**
 * pmm_init - Initializes the physical memory manager using the multiboot 
 *            memory map. Initially marks all frames as reserved, then
 *            unmarks the frames corresponding to available memory regions.
 *            Manually marks the null frame and the kernel image as reserved.
 * @mbinfo: The multiboot information structure provided by the bootloader.
 */
void __init pmm_init(multiboot_info_t* mbinfo);