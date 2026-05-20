#include "compiler.h"
#include "config.h"
#include "kernel/panic.h"
#include "kernel/services.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "loader/loader.h"
#include "mm/page.h"
#include "mm/vmm.h"

extern u8 _binary_build_services_vga_bin_start[];
extern u8 _binary_build_services_vga_bin_end[];

#define SERVICE_STACK_PG_CNT 4u                                    /* Service stack size */
#define SERVICE_GUARD_PG_CNT 1u                                    /* Guard page size    */
#define EFLAGS(iopl)         (0x202u | (((u32)(iopl) & 3u) << 12)) /* EFLAGS with IOPL   */
#define SERVICES(X)                                                                        \
    X(vga, _binary_build_services_vga_bin_start, _binary_build_services_vga_bin_end, 1, 3)

typedef struct service_t {
    const char* name;     /* Service name        */
    const u8*   start;    /* ELF image start     */
    const u8*   end;      /* ELF image end       */
    u8          priority; /* Scheduler priority  */
    u8          iopl;     /* I/O privilege level */
} service_t;

static virt_addr_t     nxt_stack_top = HIGHER_HALF_OFFSET;
static const service_t services[] = {
    #define SERVICE(name, start, end, priority, iopl) { \
        #name,                                          \
        start,                                          \
        end,                                            \
        priority,                                       \
        iopl                                            \
    },
    SERVICES(SERVICE)
    #undef SERVICE
};
static const size_t    cnt = sizeof(services) / sizeof(service_t);

static inline void map_user_stack(virt_addr_t top, u32 pg_cnt) {
    for (u32 i = 0; i < pg_cnt; i++)
        vmm_demand_map(top - (i + 1) * PG_SZ, PG_FLAG_RW | PG_FLAG_USER);
}

static task_ctrl_blk_t* server_init(const service_t* service, virt_addr_t user_stack_top) {
    size_t sz = (size_t)(service->end - service->start);

    phys_addr_t cr3 = vmm_create_aspace();
    if (unlikely(!cr3))
        return NULL;

    task_ctrl_blk_t* task = task_create(cr3);
    if (unlikely(!task))
        vmm_destroy_aspace(cr3);

    virt_addr_t entry = 0;
    ENTER_CRIT_SEC(flags);

    /* Save old address space */
    phys_addr_t saved;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(saved) : : "memory");

    /* Load new address space */
    __asm__ volatile ("mov %0, %%cr3" : : "r"(cr3) : "memory");

    entry = load_elf_from_memory(service->start, sz);
    if (likely(entry))
        map_user_stack(user_stack_top, SERVICE_STACK_PG_CNT);

    /* Restore old address space */
    __asm__ volatile ("mov %0, %%cr3" : : "r"(saved) : "memory");
    EXIT_CRIT_SEC(flags);

    if (unlikely(!entry)) {
        task_destroy(task);
        vmm_destroy_aspace(cr3);
        return NULL;
    }

    if (unlikely(!user_thread_create(task, entry, user_stack_top, EFLAGS(service->iopl), service->priority))) {
        task_destroy(task);
        vmm_destroy_aspace(cr3);
        return NULL;
    }

    return task;
}

static inline virt_addr_t alloc_user_stack(void) {
    u32 total_pg_cnt = SERVICE_STACK_PG_CNT + SERVICE_GUARD_PG_CNT;
    u32 span = total_pg_cnt * PG_SZ;
    if (nxt_stack_top <= span)
        return 0;

    nxt_stack_top -= span;
    return nxt_stack_top + PG_SZ;
}

void services_init(void) {
    for (i32 i = 0; i < cnt; i++) {
        const service_t* service = &services[i];

        virt_addr_t user_stack_top = alloc_user_stack();
        if (!user_stack_top)
            PANIC("Error: Failed to allocate service stack");

        if (!server_init(service, user_stack_top))
            PANIC("Error: Failed to start service");
    }
}