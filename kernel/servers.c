#include <stdbool.h>

#include "bits.h"
#include "boot/multiboot.h"
#include "kernel/panic.h"
#include "kernel/servers.h"
#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "libk/string.h"
#include "loader/loader.h"
#include "mm/page.h"
#include "mm/vmm.h"
#include "uapi/errno.h"
#include "uapi/mm.h"

#define STACK_PG_CNT 4u                                    /* Stack page count */
#define GUARD_PG_CNT 1u                                    /* Guard page count */
#define EFLAGS(iopl) (0x202u | (((u32)(iopl) & 3u) << 12)) /* EFLAGS with IOPL */

/* X(name, priority, iopl) */
#define SERVERS(X)    \
    X(vga, 1, 3)      \
    X(keyboard, 1, 3)

typedef struct server_info_s {
    const char* name;     /* Server name         */
    const u8*   start;    /* ELF image start     */
    const u8*   end;      /* ELF image end       */
    u32         task_id;  /* Task ID             */
    u8          priority; /* Scheduler priority  */
    u8          iopl;     /* I/O privilege level */
} server_info_s;

static virt_addr_t   nxt_stack_top = HIGHER_HALF_OFFSET;
static server_info_s servers[] = {
    #define SERVER(name, priority, iopl) { #name, NULL, NULL, 0, priority, iopl },
    SERVERS(SERVER)
    #undef SERVER
};
static const size_t  servers_cnt = sizeof(servers) / sizeof(server_info_s);

static void map_range(phys_addr_t start, phys_addr_t end) {
    if (end <= start)
        return;

    for (phys_addr_t paddr = ALIGN_DOWN_TO(start, PG_SZ); paddr < ALIGN_UP_TO(end, PG_SZ); paddr += PG_SZ)
        vmm_map_pg(paddr, __va(paddr), PG_RW_FLAG);
}

static void map_grub_modules(const multiboot_info_t* mbinfo) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MODS))
        return;

    map_range(mbinfo->mods_addr, mbinfo->mods_addr + mbinfo->mods_count * sizeof(multiboot_module_t));

    const multiboot_module_t* mods = (const multiboot_module_t*)__va(mbinfo->mods_addr);
    for (u32 i = 0; i < mbinfo->mods_count; i++) {
        map_range(mods[i].mod_start, mods[i].mod_end);
        if (mods[i].cmdline)
            map_range(mods[i].cmdline, mods[i].cmdline + PG_SZ);
    }
}

static const multiboot_module_t* find_grub_module(const multiboot_info_t* mbinfo, const char* name) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MODS))
        return NULL;

    const multiboot_module_t* mods = (const multiboot_module_t*)__va(mbinfo->mods_addr);
    for (u32 i = 0; i < mbinfo->mods_count; i++) {
        const char* cmdline = (const char*)__va(mods[i].cmdline);
        if (strcmp(cmdline, name) == 0)
            return &mods[i];
    }

    return NULL;
}

static inline void map_user_stack(virt_addr_t top, u32 pg_cnt) {
    for (u32 i = 0; i < pg_cnt; i++)
        vmm_demand_map(top - (i + 1) * PG_SZ, PG_RW_FLAG | PG_USER_FLAG);
}

static inline virt_addr_t alloc_user_stack(void) {
    const u32 total_pg_cnt = STACK_PG_CNT + GUARD_PG_CNT;
    const u32 span = total_pg_cnt * PG_SZ;
    if (nxt_stack_top <= span)
        return 0;

    nxt_stack_top -= span;
    return nxt_stack_top + PG_SZ;
}

static task_ctrl_blk_s* server_task_create(const server_info_s* server, virt_addr_t user_stack_top, virt_addr_t* out) {
    const size_t sz = (size_t)(server->end - server->start);

    phys_addr_t cr3 = vmm_create_aspace();
    if (unlikely(!cr3))
        return NULL;

    task_ctrl_blk_s* task = task_create(cr3);
    if (unlikely(!task)) {
        vmm_destroy_aspace(cr3);
        return NULL;
    }

    ENTER_CRIT_SEC(flags);
    /* Save old address space */
    phys_addr_t saved;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(saved) : : "memory");

    /* Load new address space */
    __asm__ volatile ("mov %0, %%cr3" : : "r"(cr3) : "memory");

    *out = load_elf_from_memory(server->start, sz);
    if (likely(*out))
        map_user_stack(user_stack_top, STACK_PG_CNT);

    /* Restore old address space */
    __asm__ volatile ("mov %0, %%cr3" : : "r"(saved) : "memory");
    EXIT_CRIT_SEC(flags);

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

i32 server_lookup(const char* name) {
    for (size_t i = 0; i < servers_cnt; i++) {
        if (servers[i].task_id == 0)
            continue;

        if (strcmp(servers[i].name, name) == 0)
            return (i32)servers[i].task_id;
    }

    return -(i32)E_INVAL;
}

void servers_init(const multiboot_info_t* mbinfo) {
    map_grub_modules(mbinfo);
    for (i32 i = 0; i < (i32)servers_cnt; i++) {
        server_info_s* server = &servers[i];
        const multiboot_module_t* mod = find_grub_module(mbinfo, server->name);
        if (!mod)
            PANIC("Error: Failed to find multiboot module");

        server->start = (const u8*)__va(mod->mod_start);
        server->end = (const u8*)__va(mod->mod_end);

        virt_addr_t user_stack_top = alloc_user_stack();
        if (!user_stack_top)
            PANIC("Error: Failed to allocate user stack");

        virt_addr_t entry = 0;
        task_ctrl_blk_s* task = server_task_create(server, user_stack_top, &entry);
        if (!task)
            PANIC("Error: Failed to create server task");

        server->task_id = task->id;
        server_thread_create(task, entry, user_stack_top, server->iopl, server->priority);
    }
}