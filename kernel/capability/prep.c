#include <bits.h>
#include <boot/multiboot.h>
#include <config.h>
#include <dev/console.h>
#include <kernel/capability/capability.h>
#include <kernel/core/initcall.h>
#include <kernel/core/panic.h>
#include <kernel/core/task.h>
#include <kernel/core/thread.h>
#include <libk/string.h>
#include <loader/loader.h>
#include <mm.h>
#include <mm/page.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <uapi/bootinfo.h>
#include <uapi/errno.h>
#include <uapi/servers.h>

#define ROOT_RAM_FRM_CNT  1024u    /* Root untyped RAM frame count     */
#define ROOT_RAM_NBITS    22u      /* log2(ROOT_RAM_FRM_CNT * PG_SZ)   */
#define ROOT_STACK_PG_CNT 4u       /* Root task stack page count       */
#define ROOT_PRIORITY     8u       /* Root task scheduling priority    */
#define VGA_PHYS_ADDR     0xB8000u /* VGA framebuffer physical address */
#define VGA_UNTYPED_NBITS 12u      /* log2(VGA framebuffer page size)  */

typedef struct boot_obj_s {
    u32               slot;  /* One of BOOT_SLOT_* */
    capability_type_e type;  /* Type to retype to  */
    u32               nbits; /* log2(object size)  */
} boot_obj_s;

static const boot_obj_s objs[] = {
    { BOOT_SLOT_VSPACE,   CAPABILITY_TYPE_PG_DIR,   0 },
    { BOOT_SLOT_IRQ_CTRL, CAPABILITY_TYPE_IRQ_CTRL, 0 },
};

static bootinfo_s info;
static cte_s*     rslots;
static u32        nxt = BOOT_SLOT_DYNAMIC;

extern phys_addr_t mbi;

static void __init map_untyped_frms(phys_addr_t paddr, u32 frms) {
    for (u32 i = 0; i < frms; i++)
        vmm_map_pg(paddr + i * PG_SZ, __va(paddr + i * PG_SZ), PG_RW_FLAG);
}

static void __init map_phys_range(phys_addr_t start, phys_addr_t end) {
    for (phys_addr_t paddr = ALIGN_DOWN_TO(start, PG_SZ); paddr < ALIGN_UP_TO(end, PG_SZ); paddr += PG_SZ)
        vmm_map_pg(paddr, __va(paddr), PG_RW_FLAG);
}

static bool __init is_root_mod(u32 cmdline) {
    if (!cmdline)
        return false;

    map_phys_range(cmdline, cmdline + PG_SZ);
    return strcmp((char*)__va(cmdline), "root") == 0;
}

static void __init register_multiboot_mod(multiboot_module_t* mod) {
    if (info.nmods >= BOOTINFO_MAX_MOD_CNT)
        PANIC("Error: Invalid module count");

    if (mod->mod_start & (PG_SZ - 1u))
        PANIC("Error: Invalid module start address");

    map_phys_range(mod->mod_start, mod->mod_end);
    size_t sz = (size_t)(mod->mod_end - mod->mod_start);
    const server_desc_s* desc = load_server_desc((void*)__va(mod->mod_start), sz);
    if (!desc || desc->id >= SERVER_ID_CNT)
        PANIC("Error: Invalid module descriptor");

    u32 pgs = (ALIGN_UP_TO(mod->mod_end, PG_SZ) - mod->mod_start) / PG_SZ;
    u32 first = nxt;
    for (u32 i = 0; i < pgs; i++)
        rslots[nxt++].capability = capability_mk_obj(
            CAPABILITY_TYPE_FRM,
            (void*)__va(mod->mod_start + i * PG_SZ),
            CAPABILITY_RIGHTS_ALL,
            0
        );

    mod_s* rec = &info.mods[info.nmods++];
    rec->desc  = *desc;
    rec->frm   = first;
    rec->nfrms = pgs;
    rec->sz    = (u32)sz;
}

static void __init register_multiboot_mods(multiboot_info_t* mbinfo) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MODS) || mbinfo->mods_count == 0)
        PANIC("Error: Failed to find multiboot modules");

    map_phys_range(
        mbinfo->mods_addr,
        mbinfo->mods_addr + mbinfo->mods_count * sizeof(multiboot_module_t)
    );

    multiboot_module_t* mods = (multiboot_module_t*)__va(mbinfo->mods_addr);
    for (u32 i = 0; i < mbinfo->mods_count; i++)
        if (!is_root_mod(mods[i].cmdline))
            register_multiboot_mod(&mods[i]);
}

static multiboot_module_t* __init get_root_mod(multiboot_info_t* mbinfo) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MODS) || mbinfo->mods_count == 0)
        return NULL;

    multiboot_module_t* mods = (multiboot_module_t*)__va(mbinfo->mods_addr);
    for (u32 i = 0; i < mbinfo->mods_count; i++)
        if (is_root_mod(mods[i].cmdline))
            return &mods[i];

    return NULL;
}

static void __init root_task_init(void) {
    multiboot_module_t* rmod = get_root_mod((multiboot_info_t*)__va(mbi));
    if (!rmod)
        PANIC("Error: No root task module");

    map_phys_range(rmod->mod_start, rmod->mod_end);
    u8* start = (u8*)__va(rmod->mod_start);
    size_t sz = (size_t)(rmod->mod_end - rmod->mod_start);

    task_ctrl_blk_s* task = task_create((phys_addr_t)rslots[BOOT_SLOT_VSPACE].capability.obj);
    if (!task)
        PANIC("Error: Failed to create root task");

    virt_addr_t root_stack_top = BOOTINFO_VADDR, entry;

    phys_addr_t saved;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved) : : "memory");
    __asm__ volatile("mov %0, %%cr3" : : "r"(task->cr3) : "memory");

    entry = load_elf32(start, sz);
    if (!entry)
        PANIC("Error: Failed to load root task ELF");

    for (u32 i = 0; i < ROOT_STACK_PG_CNT; i++)
        vmm_demand_map(root_stack_top - (i + 1) * PG_SZ, PG_RW_FLAG | PG_USER_FLAG);

    vmm_demand_map(BOOTINFO_VADDR, PG_RW_FLAG | PG_USER_FLAG);
    *(bootinfo_s*)BOOTINFO_VADDR = info;

    __asm__ volatile("mov %0, %%cr3" : : "r"(saved) : "memory");

    if (!user_thread_create(task, entry, root_stack_top, rslots[BOOT_SLOT_CNODE].capability, 0x202u, ROOT_PRIORITY))
        PANIC("Error: Failed to create root task thread");
}

static void __init root_cspace_init(void) {
    phys_addr_t untyped = pmm_alloc_frms(ROOT_RAM_FRM_CNT);
    if (!untyped)
        PANIC("Error: Failed to allocate root untyped memory");
    
    /* Do all frames have a corresponding higher half offset? */
    map_untyped_frms(untyped, ROOT_RAM_FRM_CNT);

    cte_s ram;
    cte_init(&ram);
    ram.capability = capability_mk_untyped((void*)__va(untyped), ROOT_RAM_NBITS);

    cte_s cnode;
    cte_init(&cnode);
    if (capability_retype(&ram, CAPABILITY_TYPE_CNODE, ROOT_CNODE_RADIX, &cnode, 1) != E_OK)
        PANIC("Error: Failed to retype root CNode");

    rslots = (cte_s*)cnode.capability.obj;
    for (u32 i = 0; i < ARRAY_SZ(objs); i++)
        if (capability_retype(&ram, objs[i].type, objs[i].nbits, &rslots[objs[i].slot], 1) != E_OK)
            PANIC("Error: Failed to retype boot capability");

    if (capability_mv(&rslots[BOOT_SLOT_CNODE], &cnode) != E_OK || capability_mv(&rslots[BOOT_SLOT_RAM], &ram) != E_OK)
        PANIC("Error: Failed to relocate root capability");

    rslots[BOOT_SLOT_VGA_UNTYPED].capability = capability_mk_untyped((void*)__va(VGA_PHYS_ADDR), VGA_UNTYPED_NBITS);

    register_multiboot_mods((multiboot_info_t*)__va(mbi));
    info.magic     = BOOTINFO_MAGIC;
    info.empty     = nxt;
    info.nempty = (1u << ROOT_CNODE_RADIX) - nxt;

    thread_self()->root = rslots[BOOT_SLOT_CNODE].capability;
    console_puts(ALL_FLAG, "[M1] Initialized root CSpace\n");
    root_task_init();
}

user_initcall(root_cspace_init);
