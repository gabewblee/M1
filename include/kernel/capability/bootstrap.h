#pragma once

#include <config.h>
#include <kernel/capability/capability.h>

#define ROOT_CNODE_RADIX       12u   /* Root CSpace slot count log2 (4096 slots) */
#define ROOT_UNTYPED_FRAMES    1024u /* Frames reserved for root untyped         */
#define ROOT_UNTYPED_SIZE_BITS 22u   /* log2(ROOT_UNTYPED_FRAMES * PG_SZ)        */

#define BOOT_SLOT_NULL         0u    /* Always-empty null slot                   */
#define BOOT_SLOT_CNODE        1u    /* Root CSpace self capability              */
#define BOOT_SLOT_VSPACE       2u    /* Root page directory                      */
#define BOOT_SLOT_IRQ_CONTROL  3u    /* IRQ allocator                            */
#define BOOT_SLOT_UNTYPED      4u    /* Initial untyped memory                   */
#define BOOT_SLOT_VGA_FB       5u    /* VGA framebuffer device frame             */
#define BOOT_SLOT_DYNAMIC      6u    /* First slot filled at boot time           */

typedef struct bootinfo_s {
    capability_s root_cnode;  /* Root CSpace root capability         */
    u32          cnode_radix; /* Root CNode radix                    */
    u32          untyped;     /* Untyped capability pointer          */
    phys_addr_t  untyped_pa;  /* Untyped physical base               */
    u32          untyped_sz;  /* Untyped size log2                   */
    u32          empty_first; /* First empty slot capability pointer */
    u32          empty_cnt;   /* Number of empty slots               */
} bootinfo_s;

/**
 * bootinfo - Returns the root task boot information.
 * Returns: The boot information describing the root CSpace.
 */
const bootinfo_s* bootinfo(void);

/**
 * bootstrap_init - Constructs the root task's CSpace and boot information.
 */
void __init bootstrap_init(void);
