#include <kernel/core/panic.h>
#include <kernel/core/sched.h>
#include <kernel/core/task.h>
#include <kernel/core/thread.h>
#include <kernel/ipc/ipc.h>
#include <kernel/sync/spinlock.h>
#include <libk/list.h>
#include <libk/string.h>
#include <mm/kheap.h>
#include <stddef.h>

thread_ctrl_blk_s* running;

static spinlock_s         lk;
static thread_ctrl_blk_s* threads[THREAD_MAX_CNT];

extern u8   gdt_start[];
extern u8   gdt_kernel_code_seg_desc[];
extern u8   gdt_user_code_seg_desc[];
extern u8   gdt_user_data_seg_desc[];
extern u8   kernel_stack_top[];
extern void kernel_thread_entry_trampoline(void);
extern void user_thread_entry_trampoline(void);

static inline i32 alloc_thread_id(thread_ctrl_blk_s* thread) {
    for (u32 id = 1; id < THREAD_MAX_CNT; id++) {
        if (!threads[id]) {
            threads[id] = thread;
            thread->id = id;
            return (i32)id;
        }
    }
    return -1;
}

static u32* kernel_stack_init(u32* top, virt_addr_t entry) {
    u32* esp = top;
    *(--esp) = 0x202;                                       /* EFLAGS                    */
    *(--esp) = (u32)(gdt_kernel_code_seg_desc - gdt_start); /* CS                        */
    *(--esp) = (u32)entry;                                  /* EIP                       */
    *(--esp) = (u32)kernel_thread_entry_trampoline;         /* Kernel thread entry point */
    *(--esp) = 0;                                           /* EBX                       */
    *(--esp) = 0;                                           /* ESI                       */
    *(--esp) = 0;                                           /* EDI                       */
    *(--esp) = 0;                                           /* EBP                       */
    return esp;
}

static u32* user_stack_init(u32* top, virt_addr_t entry, virt_addr_t uesp, u32 eflags) {
    u32* esp = top;
    *(--esp) = (u32)(gdt_user_data_seg_desc - gdt_start) | 3u; /* SS                      */
    *(--esp) = (u32)uesp;                                      /* ESP                     */
    *(--esp) = eflags;                                         /* EFLAGS                  */
    *(--esp) = (u32)(gdt_user_code_seg_desc - gdt_start) | 3u; /* CS                      */
    *(--esp) = (u32)entry;                                     /* EIP                     */
    *(--esp) = (u32)user_thread_entry_trampoline;              /* User thread entry point */
    *(--esp) = 0;                                              /* EBX                     */
    *(--esp) = 0;                                              /* ESI                     */
    *(--esp) = 0;                                              /* EDI                     */
    *(--esp) = 0;                                              /* EBP                     */
    return esp;
}

static thread_ctrl_blk_s* thread_create(task_ctrl_blk_s* task,
                                        u8*              stack0,
                                        virt_addr_t      esp,
                                        u8               priority) {
    thread_ctrl_blk_s* thread = (thread_ctrl_blk_s*)kmalloc(sizeof(thread_ctrl_blk_s));
    if (unlikely(!thread)) { 
        kfree(stack0);
        return NULL;
    }

    port_s* reply_port = port_create(1);
    if (unlikely(!reply_port)) {
        kfree(stack0);
        kfree(thread);
        return NULL;
    }

    *thread = (thread_ctrl_blk_s) {
        .esp0       = (virt_addr_t)(stack0 + THREAD_KSTACK_SZ),
        .esp        = esp,
        .cr3        = task->cr3,
        .id         = 0,
        .stack0     = stack0,
        .task       = task,
        .reply_port = reply_port,
        .priority   = priority,
        .quantum    = SCHED_QUANTUM,
        .state      = THREAD_STATE_READY,
    };

    list_init(&thread->task_link);
    list_init(&thread->run_link);
    list_init(&thread->wait_link);

    {
        spin_guard(&lk);
        if (unlikely(alloc_thread_id(thread) < 0)) {
            port_destroy(reply_port);
            kfree(stack0);
            kfree(thread);
            return NULL;
        }
        list_add_to_tail(&thread->task_link, &task->threads);
        task->nthreads++;
    }

    sched_ready(thread);
    return thread;
}

thread_ctrl_blk_s* thread_lookup(u32 id) {
    return (unlikely(id >= THREAD_MAX_CNT)) ? NULL : threads[id];
}

thread_ctrl_blk_s* thread_self(void) {
    return running;
}

void __noreturn thread_exit(void) {
    __asm__ volatile ("cli");
    thread_ctrl_blk_s* cur = thread_self();
    threads[cur->id] = NULL;
    sched_zombify();
    __builtin_unreachable();
}

thread_ctrl_blk_s* kernel_thread_create(task_ctrl_blk_s* task, virt_addr_t entry, u8 priority) {
    u8* stack0 = (u8*)kzalloc(THREAD_KSTACK_SZ);
    if (unlikely(!stack0))
        return NULL;

    virt_addr_t esp = (virt_addr_t)kernel_stack_init((u32*)(stack0 + THREAD_KSTACK_SZ), entry);
    return thread_create(task, stack0, esp, priority);
}

thread_ctrl_blk_s* user_thread_create(task_ctrl_blk_s* task,
                                      virt_addr_t      entry,
                                      virt_addr_t      user_stack_top,
                                      u32              eflags, 
                                      u8               priority) {
    u8* stack0 = (u8*)kzalloc(THREAD_KSTACK_SZ);
    if (unlikely(!stack0))
        return NULL;

    virt_addr_t esp = (virt_addr_t)user_stack_init((u32*)(stack0 + THREAD_KSTACK_SZ), entry, user_stack_top, eflags);
    return thread_create(task, stack0, esp, priority);
}

void thread_destroy(thread_ctrl_blk_s* thread) {
    if (unlikely(!thread_lookup(thread->id)))
        return;

    if (list_is_attached(&thread->wait_link))
        list_del(&thread->wait_link);

    if (list_is_attached(&thread->task_link))
        list_del(&thread->task_link);

    if (thread->task)
        thread->task->nthreads--;
    
    if (thread->reply_port)
        port_destroy(thread->reply_port);

    if (thread->stack0)
        kfree(thread->stack0);

    kfree(thread);
}

void __init thread0_init(void) {
    spinlock_init(&lk);

    thread_ctrl_blk_s* thread0 = kmalloc(sizeof(thread_ctrl_blk_s));
    if (unlikely(!thread0))
        PANIC("Error: Failed to initialize thread0");

    u32 esp;
    __asm__ volatile ("mov %%esp, %0" : "=r"(esp));

    task_ctrl_blk_s* task0 = task_lookup(0);
    
    *thread0 = (thread_ctrl_blk_s) {
        .esp0     = (virt_addr_t)kernel_stack_top,
        .esp      = (virt_addr_t)esp,
        .cr3      = task0->cr3,
        .id       = 0,
        .task     = task0,
        .state    = THREAD_STATE_RUNNING,
        .priority = SCHED_IDLE_PRIORITY,
        .quantum  = SCHED_QUANTUM
    };

    list_init(&thread0->task_link);
    list_add_to_tail(&thread0->task_link, &task0->threads);
    task0->nthreads++;

    list_init(&thread0->run_link);
    list_init(&thread0->wait_link);
    
    threads[0] = thread0;
    running = thread0;
}
