#pragma once

#include <types.h>

#define USER_BOOTINFO_VADDR  0xBFFFF000u /* Where the kernel maps the bootinfo page  */
#define USER_BOOTINFO_MAGIC  0x600DC0DEu /* Magic stamped into a valid bootinfo page */
#define ROOT_CNODE_DEPTH     12u         /* Bits the root CNode resolves per cptr    */
#define BOOTINFO_MAX_MODULES 8u          /* Boot modules the bootinfo page can carry */

/**
 * bootinfo_module_s - Describes one server boot module handed to the root task.
 *                     The module's ELF image is exposed as @frame_count Frame
 *                     capabilities at consecutive root CNode slots beginning at
 *                     @frame_first; the root task maps them to read the image.
 */
typedef struct bootinfo_module_s {
    u32 server_id;   /* server_id_e of this module                 */
    u32 priority;    /* Scheduler priority from the module note    */
    u32 iopl;        /* I/O privilege level from the module note   */
    u32 frame_first; /* Capability pointer of the first image Frame */
    u32 frame_count; /* Number of page Frames spanning the image   */
    u32 image_size;  /* Image size in bytes                        */
} bootinfo_module_s;

/**
 * user_bootinfo_s - The boot information the kernel hands the root task. It is
 *                   mapped at USER_BOOTINFO_VADDR in the root task's address
 *                   space before it begins executing. The root task is the
 *                   system's root server: it retypes its untyped memory into
 *                   the objects every other server needs, loads the server
 *                   images from their Frame capabilities, and distributes the
 *                   capabilities that wire the system together.
 */
typedef struct user_bootinfo_s {
    u32               magic;             /* USER_BOOTINFO_MAGIC if the page is valid    */
    u32               cnode_radix;       /* Root CNode radix (== resolve depth)         */
    u32               untyped;           /* Initial untyped capability pointer          */
    u32               untyped_size_bits; /* Initial untyped size log2                   */
    u32               empty_first;       /* First empty root CNode slot cptr            */
    u32               empty_cnt;         /* Number of empty root CNode slots            */
    u32               slot_cnode;        /* Root CSpace self capability pointer         */
    u32               slot_vspace;       /* Root VSpace (page directory) cptr           */
    u32               slot_irq_control;  /* IRQ control capability pointer              */
    u32               vga_fb_frame;      /* VGA framebuffer device Frame cptr, 0 if none */
    u32               module_cnt;        /* Number of boot modules below                */
    bootinfo_module_s modules[BOOTINFO_MAX_MODULES]; /* Server boot modules             */
} user_bootinfo_s;
