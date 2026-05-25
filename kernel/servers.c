#include "compiler.h"
#include "config.h"
#include "kernel/panic.h"
#include "kernel/servers.h"
#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "lib/string.h"
#include "loader/loader.h"
#include "mm/page.h"
#include "mm/vmm.h"
#include "uapi.h"

extern const u8 _binary_build_servers_vga_bin_start[];
extern const u8 _binary_build_servers_vga_bin_end[];

#define STACK_PG_CNT 4u                                    /* Stack page count */
#define GUARD_PG_CNT 1u                                    /* Guard page count */
#define EFLAGS(iopl) (0x202u | (((u32)(iopl) & 3u) << 12)) /* EFLAGS with IOPL */

/* X(name, start, end, priority, iopl) */
#define SERVERS(X) \
    X(vga, _binary_build_servers_vga_bin_start, _binary_build_servers_vga_bin_end, 1, 3)

typedef struct server_info_t {
    const char* name;     /* Server name         */
    const u8*   start;    /* ELF image start     */
    const u8*   end;      /* ELF image end       */
    u8          priority; /* Scheduler priority  */
    u8          iopl;     /* I/O privilege level */
    u32         task_id;  /* Task ID             */
} server_info_t;

static virt_addr_t  nxt_stack_top = HIGHER_HALF_OFFSET;
static server_info_t servers[] = {
    #define SERVER(name, start, end, priority, iopl) { \
        #name, start, end, priority, iopl, 0           \
    },
    SERVERS(SERVER)
    #undef SERVER
};
static const size_t servers_cnt = sizeof(servers) / sizeof(server_info_t);

static inline virt_addr_t alloc_user_stack(void) {
    u32 total_pg_cnt = STACK_PG_CNT + GUARD_PG_CNT;
    u32 span = total_pg_cnt * PG_SZ;
    if (nxt_stack_top <= span)
        return 0;

    nxt_stack_top -= span;
    return nxt_stack_top + PG_SZ;
}

static inline void map_user_stack(virt_addr_t top, u32 pg_cnt) {
    for (u32 i = 0; i < pg_cnt; i++)
        vmm_demand_map(top - (i + 1) * PG_SZ, PG_FLAG_RW | PG_FLAG_USER);
}

static task_ctrl_blk_t* server_task_create(const server_info_t* server, virt_addr_t user_stack_top, virt_addr_t* out) {
    size_t sz = (size_t)(server->end - server->start);

    phys_addr_t cr3 = vmm_create_aspace();
    if (unlikely(!cr3))
        return NULL;

    task_ctrl_blk_t* task = task_create(cr3);
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

static bool server_thread_create(task_ctrl_blk_t* task,
                                 virt_addr_t      entry,
                                 virt_addr_t      user_stack_top,
                                 u8               iopl,
                                 u8               priority) {
    if (unlikely(!user_thread_create(task, entry, user_stack_top, EFLAGS(iopl), priority))) {
        task_destroy(task);
        vmm_destroy_aspace(task->cr3);
        return false;
    }

    return true;
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

void servers_init(void) {
    for (i32 i = 0; i < (i32)servers_cnt; i++) {
        const server_info_t* server = &servers[i];

        virt_addr_t user_stack_top = alloc_user_stack();
        if (!user_stack_top)
            PANIC("Error: Failed to allocate server stack");

        virt_addr_t entry = 0;
        task_ctrl_blk_t* task = server_task_create(server, user_stack_top, &entry);
        if (!task)
            PANIC("Error: Failed to load server");

        servers[i].task_id = task->id;
        if (!server_thread_create(task, entry, user_stack_top, server->iopl, server->priority))
            PANIC("Error: Failed to start server");
    }
}
