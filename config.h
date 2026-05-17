#pragma once

#include <stdint.h>

#define VGA_BLACK_COLOR                     0x00
#define VGA_WHITE_COLOR                     0x0F
#define VGA_PHYS_ADDR                       0xB8000

#define PG_SH                               12
#define PG_SZ                               (1u << PG_SH)
#define pg_shl(x)                           ((x) << PG_SH)
#define pg_shr(x)                           ((x) >> PG_SH)
#define one_shl(x)                          (1u << x)

#define get_container_of(ptr, type, member) ((type *)((uintptr_t)(ptr) - offsetof(type, member)))

#define HIGHER_HALF_OFFSET                  0xC0000000
#define __va(x)                             ((virt_addr_t)((uintptr_t)(x) + HIGHER_HALF_OFFSET))
#define __pa(x)                             ((phys_addr_t)((uintptr_t)(x) - HIGHER_HALF_OFFSET))

#define __multiboot                         __attribute__((__section__(".multiboot")))
#define __init                              __attribute__((__section__(".init")))
#define __noreturn                          __attribute__((__noreturn__))
#define __hot                               __attribute__((__hot__))
#define __cold                              __attribute__((__cold__))
#define __always_inline inline              __attribute__((__always_inline__))

#define __packed                            __attribute__((__packed__))
#define __aligned(x)                        __attribute__((__aligned__(x)))

#define likely(x)                           __builtin_expect(!!(x), 1)
#define unlikely(x)                         __builtin_expect(!!(x), 0)

#define enter_crit_sec(flags) \
    u32 flags;                \
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory")

#define exit_crit_sec(flags)  \
    __asm__ volatile("push %0; popf" : : "r"(flags) : "memory", "cc")

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;
typedef uint32_t phys_addr_t;
typedef uint32_t virt_addr_t;