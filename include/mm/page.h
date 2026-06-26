#pragma once

#define PFN_TO_PHYS(x)     ((x) << 12u)                                         /* Converts a page frame number to a physical address           */
#define PHYS_TO_PFN(x)     ((x) >> 12u)                                         /* Converts a physical address to a page frame number           */

#define HIGHER_HALF_OFFSET 0xC0000000u                                          /* Higher-half offset                                           */
#define __va(x)            ((virt_addr_t)((uintptr_t)(x) + HIGHER_HALF_OFFSET)) /* Converts a physical address to a higher-half virtual address */
#define __pa(x)            ((phys_addr_t)((uintptr_t)(x) - HIGHER_HALF_OFFSET)) /* Converts a higher-half virtual address to a physical address */
