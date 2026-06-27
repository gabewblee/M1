#include <bits.h>
#include <boot/multiboot.h>
#include <kernel/core/initcall.h>
#include <kernel/core/panic.h>
#include <kernel/core/task.h>
#include <kernel/core/thread.h>
#include <kernel/ipc/servers.h>
#include <kernel/sync/spinlock.h>
#include <kernel/syscall.h>
#include <libk/string.h>
#include <loader/loader.h>
#include <mm/page.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <uapi/errno.h>
#include <uapi/mm.h>
#include <uapi/servers.h>

#define STACK_PG_CNT 4u                                    /* Stack page count */
#define GUARD_PG_CNT 1u                                    /* Guard page count */
#define EFLAGS(iopl) (0x202u | (((u32)(iopl) & 3u) << 12)) /* EFLAGS with IOPL */

typedef struct server_info_s {
    u8* start;    /* ELF image start     */
    u8* end;      /* ELF image end       */
    u32 task_id;  /* Task ID             */
    u8  priority; /* Scheduler priority  */
    u8  iopl;     /* I/O privilege level */
} server_info_s;

static spinlock_s    lk;
static virt_addr_t   nxt_stack_top = HIGHER_HALF_OFFSET;
static server_info_s servers[SERVER_ID_CNT];

extern phys_addr_t mbi;

static void map_range(phys_addr_t start, phys_addr_t end) {
    for (phys_addr_t paddr = ALIGN_DOWN_TO(start, PG_SZ); paddr < ALIGN_UP_TO(end, PG_SZ); paddr += PG_SZ)
        vmm_map_pg(paddr, __va(paddr), PG_RW_FLAG);
}

static void map_multiboot_mods(multiboot_info_t* mbinfo) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MODS))
        return;

    map_range(mbinfo->mods_addr, mbinfo->mods_addr + mbinfo->mods_count * sizeof(multiboot_module_t));
    multiboot_module_t* mods = (multiboot_module_t*)__va(mbinfo->mods_addr);
    for (u32 i = 0; i < mbinfo->mods_count; i++) {
        map_range(mods[i].mod_start, mods[i].mod_end);
        if (mods[i].cmdline)
            map_range(mods[i].cmdline, mods[i].cmdline + PG_SZ);
    }
}

static inline void map_user_stack(virt_addr_t top, u32 cnt) {
    for (u32 i = 0; i < cnt; i++)
        vmm_demand_map(top - (i + 1) * PG_SZ, PG_RW_FLAG | PG_USER_FLAG);
}

static inline virt_addr_t alloc_user_stack(void) {
    u32 span = (STACK_PG_CNT + GUARD_PG_CNT) * PG_SZ;
    if (nxt_stack_top <= span)
        return 0;

    nxt_stack_top -= span;
    return nxt_stack_top + PG_SZ;
}

static task_ctrl_blk_s* server_task_create(server_info_s* server, virt_addr_t user_stack_top, virt_addr_t* out) {
    size_t sz = (size_t)(server->end - server->start);

    phys_addr_t cr3 = vmm_create_aspace();
    if (unlikely(!cr3))
        return NULL;

    task_ctrl_blk_s* task = task_create(cr3);
    if (unlikely(!task)) {
        vmm_destroy_aspace(cr3);
        return NULL;
    }

    {
        spin_guard(&lk);

        phys_addr_t saved;
        __asm__ volatile ("mov %%cr3, %0" : "=r"(saved) : : "memory");
        __asm__ volatile ("mov %0, %%cr3" : : "r"(cr3) : "memory");

        *out = load_elf32(server->start, sz);
        if (likely(*out))
            map_user_stack(user_stack_top, STACK_PG_CNT);

        __asm__ volatile ("mov %0, %%cr3" : : "r"(saved) : "memory");
    }

    if (unlikely(!*out)) {
        task_destroy(task);
        vmm_destroy_aspace(cr3);
        return NULL;
    }

    return task;
}

static void server_thread_create(task_ctrl_blk_s* task,
                                 virt_addr_t      entry,
                                 virt_addr_t      user_stack_top,
                                 u8               iopl,
                                 u8               priority) {
    if (unlikely(!user_thread_create(task, entry, user_stack_top, EFLAGS(iopl), priority))) {
        task_destroy(task);
        vmm_destroy_aspace(task->cr3);
        PANIC("Error: Failed to initialize server thread");
    }
}

i32 server_lookup(u32 id) {
    if (id >= SERVER_ID_CNT || servers[id].task_id == 0)
        return -(i32)E_INVAL;

    return (i32)servers[id].task_id;
}

void servers_init(multiboot_info_t* mbinfo) {
    map_multiboot_mods(mbinfo);
    if (!(mbinfo->flags & MULTIBOOT_INFO_MODS))
        PANIC("Error: Missing multiboot modules");

    multiboot_module_t* mods = (multiboot_module_t*)__va(mbinfo->mods_addr);
    for (u32 i = 0; i < mbinfo->mods_count; i++) {
        u8* start = (u8*)__va(mods[i].mod_start);
        u8* end = (u8*)__va(mods[i].mod_end);

        const server_desc_s* desc = load_server_desc(start, (size_t)(end - start));
        if (!desc)
            PANIC("Error: Missing descriptor note");

        if (desc->id >= SERVER_ID_CNT)
            PANIC("Error: Invalid server ID");

        server_info_s* server = &servers[desc->id];
        server->start         = start;
        server->end           = end;
        server->priority      = desc->priority;
        server->iopl          = desc->iopl;

        virt_addr_t user_stack_top = alloc_user_stack();
        if (!user_stack_top)
            PANIC("Error: Failed to allocate user stack");

        virt_addr_t entry;
        task_ctrl_blk_s* task = server_task_create(server, user_stack_top, &entry);
        if (!task)
            PANIC("Error: Failed to create server task");

        server->task_id = task->id;
        server_thread_create(task, entry, user_stack_top, server->iopl, server->priority);
    }
}

static void __init servers_initcall(void) {
    servers_init((multiboot_info_t*)__va(mbi));
}

late_initcall(servers_initcall);