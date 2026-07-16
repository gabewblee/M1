#pragma once

#include <types.h>
#include <uapi/servers.h>

#define ROOT_CNODE_RADIX     12u         /* log2(root CNode slot count)   */
#define BOOTINFO_MAGIC       0x600DC0DEu /* Bootinfo page magic number    */
#define BOOTINFO_VADDR       0xBFFFF000u /* Bootinfo page virtual address */
#define BOOTINFO_MAX_MOD_CNT 8u          /* Maximum boot module count     */

#define BOOT_SLOT_NULL         0u /* Null slot                             */
#define BOOT_SLOT_CNODE        1u /* Root CSpace capability                */
#define BOOT_SLOT_VSPACE       2u /* Root page directory                   */
#define BOOT_SLOT_IRQ_CTRL     3u /* IRQ allocator                         */
#define BOOT_SLOT_RAM          4u /* General-purpose untyped memory        */
#define BOOT_SLOT_VGA_UNTYPED  5u /* VGA framebuffer's untyped device page */
#define BOOT_SLOT_DYNAMIC      6u /* First slot free for root's own use    */

typedef struct mod_s {
    server_desc_s desc;  /* The server's self-declared descriptor (id/priority/iopl/resources) */
    u32           frm;   /* The first image frame capability pointer                           */
    u32           nfrms; /* The image frame count                                              */
    u32           sz;    /* The image size in bytes                                            */
} mod_s;

typedef struct bootinfo_s {
    u32   magic;                      /* USER_BOOTINFO_MAGIC if valid       */
    u32   empty;                      /* The first empty capability pointer */
    u32   nempty;                  /* The empty slot count               */
    mod_s mods[BOOTINFO_MAX_MOD_CNT]; /* The server boot modules            */
    u32   nmods;                    /* The boot module count              */
} bootinfo_s;
