#include <bits.h>
#include <boot/multiboot.h>
#include <config.h>
#include <dev/console.h>
#include <kernel/capability/bootstrap.h>
#include <kernel/capability/capability.h>
#include <kernel/core/initcall.h>
#include <kernel/core/panic.h>
#include <kernel/core/task.h>
#include <kernel/core/thread.h>
#include <loader/loader.h>
#include <mm.h>
#include <mm/kheap.h>
#include <mm/page.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <uapi/bootinfo.h>
#include <uapi/servers.h>

#define ROOT_STACK_PG_CNT 4u       /* Root task user stack page count */
#define ROOT_PRIORITY     8u       /* Root task scheduling priority   */
#define VGA_FB_PA         0xB8000u /* VGA text framebuffer physical   */

extern phys_addr_t mbi;

static bootinfo_s        info;
static cte_s*            root_slots;
static bootinfo_module_s modules[BOOTINFO_MAX_MODULES];
static u32               module_cnt;
static u32               next_slot = BOOT_SLOT_DYNAMIC;

const bootinfo_s* bootinfo(void) {
    return &info;
}

static void map_untyped(phys_addr_t pa, u32 frames) {
    for (u32 i = 0; i < frames; i++)
        vmm_map_pg(pa + i * PG_SZ, __va(pa + i * PG_SZ), PG_RW_FLAG);
}

static void map_phys_range(phys_addr_t start, phys_addr_t end) {
    for (phys_addr_t pa = ALIGN_DOWN_TO(start, PG_SZ); pa < ALIGN_UP_TO(end, PG_SZ); pa += PG_SZ)
        vmm_map_pg(pa, __va(pa), PG_RW_FLAG);
}

static bool cmdline_is_root(u32 cmdline_pa) {
    if (!cmdline_pa)
        return false;

    map_phys_range(cmdline_pa, cmdline_pa + PG_SZ);
    const char* s    = (const char*)__va(cmdline_pa);
    const char* want = "root";
    u32         i    = 0;
    for (; want[i]; i++)
        if (s[i] != want[i])
            return false;

    return s[i] == '\0' || s[i] == ' ';
}

static void fill_user_bootinfo(user_bootinfo_s* ubi) {
    ubi->magic             = USER_BOOTINFO_MAGIC;
    ubi->cnode_radix       = ROOT_CNODE_RADIX;
    ubi->untyped           = BOOT_SLOT_UNTYPED;
    ubi->untyped_size_bits = ROOT_UNTYPED_SIZE_BITS;
    ubi->empty_first       = info.empty_first;
    ubi->empty_cnt         = info.empty_cnt;
    ubi->slot_cnode        = BOOT_SLOT_CNODE;
    ubi->slot_vspace       = BOOT_SLOT_VSPACE;
    ubi->slot_irq_control  = BOOT_SLOT_IRQ_CONTROL;
    ubi->vga_fb_frame      = BOOT_SLOT_VGA_FB;
    ubi->module_cnt        = module_cnt;
    for (u32 i = 0; i < module_cnt; i++)
        ubi->modules[i] = modules[i];
}

/* Exposes a non-root module's ELF image to the root task as a run of Frame
   capabilities over its physical pages, and records a descriptor the root task
   uses to retype, load, and configure the server. */
static void register_module(multiboot_module_t* mod) {
    if (module_cnt >= BOOTINFO_MAX_MODULES)
        PANIC("Error: Too many boot modules");

    if (mod->mod_start & (PG_SZ - 1u))
        PANIC("Error: Boot module is not page aligned");

    map_phys_range(mod->mod_start, mod->mod_end);
    size_t sz = (size_t)(mod->mod_end - mod->mod_start);

    const server_desc_s* desc = load_server_desc((void*)__va(mod->mod_start), sz);
    if (!desc || desc->id >= SERVER_ID_CNT)
        PANIC("Error: Invalid server module descriptor");

    u32 pages = (ALIGN_UP_TO(mod->mod_end, PG_SZ) - mod->mod_start) / PG_SZ;
    u32 first = next_slot;
    for (u32 i = 0; i < pages; i++)
        root_slots[next_slot++].capability =
            capability_mk_obj(CAPABILITY_TYPE_FRM, (void*)__va(mod->mod_start + i * PG_SZ), CAPABILITY_RIGHTS_ALL, 0);

    modules[module_cnt++] = (bootinfo_module_s) {
        .server_id   = desc->id,
        .priority    = desc->priority,
        .iopl        = desc->iopl,
        .frame_first = first,
        .frame_count = pages,
        .image_size  = (u32)sz,
    };
}

static void register_modules(multiboot_info_t* mbinfo) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MODS) || mbinfo->mods_count == 0)
        PANIC("Error: Missing multiboot modules");

    map_phys_range(mbinfo->mods_addr,
                   mbinfo->mods_addr + mbinfo->mods_count * sizeof(multiboot_module_t));
    multiboot_module_t* mods = (multiboot_module_t*)__va(mbinfo->mods_addr);
    for (u32 i = 0; i < mbinfo->mods_count; i++)
        if (!cmdline_is_root(mods[i].cmdline))
            register_module(&mods[i]);
}

static multiboot_module_t* find_root_module(multiboot_info_t* mbinfo) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MODS) || mbinfo->mods_count == 0)
        return NULL;

    multiboot_module_t* mods = (multiboot_module_t*)__va(mbinfo->mods_addr);
    for (u32 i = 0; i < mbinfo->mods_count; i++)
        if (cmdline_is_root(mods[i].cmdline))
            return &mods[i];

    return NULL;
}

static void root_task_init(void) {
    multiboot_module_t* rootmod = find_root_module((multiboot_info_t*)__va(mbi));
    if (!rootmod)
        PANIC("Error: No root task module");

    map_phys_range(rootmod->mod_start, rootmod->mod_end);
    u8*    start = (u8*)__va(rootmod->mod_start);
    size_t sz    = (size_t)(rootmod->mod_end - rootmod->mod_start);

    phys_addr_t cr3 = vmm_create_aspace();
    if (!cr3)
        PANIC("Error: Failed to create root task address space");

    /* The root task's VSpace capability names its own page directory, so it can
       map frames into and configure threads for its own address space. */
    root_slots[BOOT_SLOT_VSPACE].capability.obj = (void*)cr3;

    task_ctrl_blk_s* task = task_create(cr3);
    if (!task)
        PANIC("Error: Failed to create root task");

    virt_addr_t stack_top = USER_BOOTINFO_VADDR;
    virt_addr_t entry;

    phys_addr_t saved;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved) : : "memory");
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");

    entry = load_elf32(start, sz);
    if (entry) {
        for (u32 i = 0; i < ROOT_STACK_PG_CNT; i++)
            vmm_demand_map(stack_top - (i + 1) * PG_SZ, PG_RW_FLAG | PG_USER_FLAG);

        vmm_demand_map(USER_BOOTINFO_VADDR, PG_RW_FLAG | PG_USER_FLAG);
        fill_user_bootinfo((user_bootinfo_s*)USER_BOOTINFO_VADDR);
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"(saved) : "memory");

    if (!entry)
        PANIC("Error: Failed to load root task ELF");

    if (!user_thread_create(task, entry, stack_top, info.root_cnode, 0x202u, ROOT_PRIORITY))
        PANIC("Error: Failed to create root task thread");

    console_puts(ALL_FLAG, "[CAP] Handed off to root task\n");
}

void __init bootstrap_init(void) {
    u32 cnt = 1u << ROOT_CNODE_RADIX;
    root_slots = (cte_s*)kzalloc(sizeof(cte_s) * cnt);
    if (!root_slots)
        PANIC("Error: Failed to allocate root CNode");

    cnode_init(root_slots, cnt);
    info.root_cnode  = capability_mk_cnode(root_slots, ROOT_CNODE_RADIX, 0, 0);
    info.cnode_radix = ROOT_CNODE_RADIX;

    root_slots[BOOT_SLOT_CNODE].capability = info.root_cnode;

    task_ctrl_blk_s* task0 = task_lookup(KERNEL_TASK_ID);
    root_slots[BOOT_SLOT_VSPACE].capability =
        capability_mk_obj(CAPABILITY_TYPE_PG_DIR, (void*)task0->cr3, CAPABILITY_RIGHTS_ALL, 0);

    root_slots[BOOT_SLOT_IRQ_CONTROL].capability =
        capability_mk_obj(CAPABILITY_TYPE_IRQ_CONTROL, NULL, CAPABILITY_RIGHTS_ALL, 0);

    phys_addr_t pa = pmm_alloc_frms(ROOT_UNTYPED_FRAMES);
    if (!pa)
        PANIC("Error: Failed to reserve root untyped memory");

    map_untyped(pa, ROOT_UNTYPED_FRAMES);
    root_slots[BOOT_SLOT_UNTYPED].capability = capability_mk_untyped((void*)__va(pa), ROOT_UNTYPED_SIZE_BITS);

    /* The VGA text framebuffer is device memory the root task hands to the VGA
       server as a Frame capability to map into its own address space. */
    root_slots[BOOT_SLOT_VGA_FB].capability =
        capability_mk_obj(CAPABILITY_TYPE_FRM, (void*)__va(VGA_FB_PA), CAPABILITY_RIGHTS_ALL, 0);

    register_modules((multiboot_info_t*)__va(mbi));

    info.untyped     = BOOT_SLOT_UNTYPED;
    info.untyped_pa  = pa;
    info.untyped_sz  = ROOT_UNTYPED_SIZE_BITS;
    info.empty_first = next_slot;
    info.empty_cnt   = cnt - next_slot;

    thread_self()->cspace_root = info.root_cnode;

    console_puts(ALL_FLAG, "[CAP] Constructed root CSpace\n");

    root_task_init();
}

user_initcall(bootstrap_init);
