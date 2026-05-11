#pragma once

#include <stdint.h>

#define VGA_BLACK_COLOR        0x00                                                 /* VGA black color */
#define VGA_WHITE_COLOR        0x0F                                                 /* VGA white color */
#define VGA_PHYS_ADDR          0xB8000                                              /* VGA buffer physical address */

#define PG_SH                  12                                                   /* Page size shift */
#define PG_SZ                  (1u << PG_SH)                                        /* Page size */
#define __pg_shl(x)            ((x) << PG_SH)                                       /* Multiply by page size */
#define __pg_shr(x)            ((x) >> PG_SH)                                       /* Divide by page size */

#define HIGHER_HALF_OFFSET     0xC0000000                                           /* Higher half offset */
#define __va(x)                ((virt_addr_t)((uintptr_t)(x) + HIGHER_HALF_OFFSET)) /* Convert a physical address to a virtual address */
#define __pa(x)                ((phys_addr_t)((uintptr_t)(x) - HIGHER_HALF_OFFSET)) /* Convert a virtual address to a physical address */

#define __init                 __attribute__((__section__(".init")))                /* Function attribute indicating that the function belongs in the init section */
#define __noreturn             __attribute__((__noreturn__))                        /* Function attribute indicating that the function does not return */
#define __hot                  __attribute__((__hot__))                             /* Function attribute indicating that the function should be optimized for performance */
#define __cold                 __attribute__((__cold__))                            /* Function attribute indicating that the function should be optimized for space */
#define __always_inline inline __attribute__((__always_inline__))                   /* Function attribute indicating that the function should always be inlined */

#define __packed               __attribute__((__packed__))                          /* Struct attribute indicating that the struct should be packed */
#define __aligned(x)           __attribute__((__aligned__(x)))                      /* Struct attribute indicating that the struct should be aligned to the specified boundary */

typedef uint64_t u64;                                                               /* 64-bit unsigned integer */
typedef uint32_t u32;                                                               /* 32-bit unsigned integer */ 
typedef uint16_t u16;                                                               /* 16-bit unsigned integer */
typedef uint8_t  u8;                                                                /* 8-bit unsigned integer */
typedef uint32_t phys_addr_t;                                                       /* Physical address type */
typedef uint32_t virt_addr_t;                                                       /* Virtual address type */