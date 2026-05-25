#include <stddef.h>

#include "kernel/ipc.h"
#include "kernel/panic.h"
#include "kernel/sched.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "lib/list.h"
#include "lib/string.h"
#include "mm/kheap.h"

thread_ctrl_blk_t* running;

static thread_ctrl_blk_t* threads[THREAD_MAX_CNT];

extern const u8   gdt_start[];
extern const u8   gdt_kernel_code_seg_desc[];
extern const u8   gdt_user_code_seg_desc[];
extern const u8   gdt_user_data_seg_desc[];
extern const u8   kernel_stack_top[];
extern void kernel_thread_entry_trampoline(void);
extern void user_thread_entry_trampoline(void);

static inline i32 alloc_thread_id(thread_ctrl_blk_t* thread) {
    for (u32 tid = 1; tid < THREAD_MAX_CNT; tid++) {
        if (!threads[tid]) {
            threads[tid] = thread;
            thread->tid = tid;
            return (i32)tid;
        }
    }
    return -1;
}

static void __noreturn entry_stub(void) {
    thread_self()->entry();
    thread_exit();
}

static u32* kernel_stack_init(u32* top) {
    u32* esp = top;
    *(--esp) = 0x202;                                       /* EFLAGS                    */
    *(--esp) = (u32)(gdt_kernel_code_seg_desc - gdt_start); /* CS                        */
    *(--esp) = (u32)entry_stub;                             /* EIP                       */
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

thread_ctrl_blk_t* thread_lookup(u32 tid) {
    return (unlikely(tid >= THREAD_MAX_CNT)) ? NULL : threads[tid];
}

thread_ctrl_blk_t* thread_self(void) {
    return running;
}

void __noreturn thread_exit(void) {
    __asm__ volatile ("cli");
    const thread_ctrl_blk_t* cur = thread_self();
    threads[cur->tid] = NULL;
    sched_zombify();
    __builtin_unreachable();
}

thread_ctrl_blk_t* kernel_thread_create(task_ctrl_blk_t* task, thread_entry_func_t entry, u8 priority) {
    thread_ctrl_blk_t* thread = (thread_ctrl_blk_t*)kmalloc(sizeof(thread_ctrl_blk_t));
    if (unlikely(!thread))
        return NULL;

    u8* kernel_stack = (u8*)kmalloc(THREAD_KSTACK_SZ);
    if (unlikely(!kernel_stack)) {
        kfree(thread);
        return NULL;
    }

    port_t* reply_port = port_create(1);
    if (unlikely(!reply_port)) {
        kfree(kernel_stack);
        kfree(thread);
        return NULL;
    }

    u32* esp = kernel_stack_init((u32*)(kernel_stack + THREAD_KSTACK_SZ));
    *thread = (thread_ctrl_blk_t) {
        .esp0         = (virt_addr_t)(kernel_stack + THREAD_KSTACK_SZ),
        .esp          = (virt_addr_t)esp,
        .cr3          = task->cr3,
        .tid          = 0,
        .kernel_stack = kernel_stack,
        .entry        = entry,
        .task         = task,
        .reply_port   = reply_port,
        .priority     = priority,
        .quantum      = SCHED_QUANTUM,
        .state        = THREAD_STATE_READY
    };

    list_init(&thread->task_link);
    list_init(&thread->run_link);
    list_init(&thread->wait_link);

    ENTER_CRIT_SEC(flags);
    if (unlikely(alloc_thread_id(thread) < 0)) {
        EXIT_CRIT_SEC(flags);
        port_destroy(thread->reply_port);
        kfree(kernel_stack);
        kfree(thread);
        return NULL;
    }

    list_add_to_tail(&thread->task_link, &task->threads);
    task->nthreads++;

    sched_ready(thread);
    EXIT_CRIT_SEC(flags);
    return thread;
}

thread_ctrl_blk_t* user_thread_create(task_ctrl_blk_t* task,
                                      virt_addr_t      entry,
                                      virt_addr_t      user_stack_top,
                                      u32              eflags,
                                      u8               priority) {
    thread_ctrl_blk_t* thread = (thread_ctrl_blk_t*)kmalloc(sizeof(thread_ctrl_blk_t));
    if (unlikely(!thread))
        return NULL;

    u8* kernel_stack = (u8*)kmalloc(THREAD_KSTACK_SZ);
    if (unlikely(!kernel_stack)) {
        kfree(thread);
        return NULL;
    }

    port_t* reply_port = port_create(1);
    if (unlikely(!reply_port)) {
        kfree(kernel_stack);
        kfree(thread);
        return NULL;
    }

    u32* esp = user_stack_init((u32*)(kernel_stack + THREAD_KSTACK_SZ), entry, user_stack_top, eflags);
    *thread = (thread_ctrl_blk_t) {
        .esp0         = (virt_addr_t)(kernel_stack + THREAD_KSTACK_SZ),
        .esp          = (virt_addr_t)esp,
        .cr3          = task->cr3,
        .tid          = 0,
        .kernel_stack = kernel_stack,
        .entry        = NULL,
        .task         = task,
        .reply_port   = reply_port,
        .priority     = priority,
        .quantum      = SCHED_QUANTUM,
        .state        = THREAD_STATE_READY
    };

    list_init(&thread->task_link);
    list_init(&thread->run_link);
    list_init(&thread->wait_link);

    ENTER_CRIT_SEC(flags);
    if (unlikely(alloc_thread_id(thread) < 0)) {
        EXIT_CRIT_SEC(flags);
        port_destroy(thread->reply_port);
        kfree(kernel_stack);
        kfree(thread);
        return NULL;
    }

    list_add_to_tail(&thread->task_link, &task->threads);
    task->nthreads++;

    sched_ready(thread);
    EXIT_CRIT_SEC(flags);
    return thread;
}

void thread_destroy(thread_ctrl_blk_t* thread) {
    if (!unlikely(thread_lookup(thread->tid)))
        return;

    if (list_is_attached(&thread->wait_link))
        list_del(&thread->wait_link);

    if (list_is_attached(&thread->task_link))
        list_del(&thread->task_link);

    if (thread->task)
        thread->task->nthreads--;
    
    if (thread->reply_port)
        port_destroy(thread->reply_port);

    if (thread->kernel_stack)
        kfree(thread->kernel_stack);

    kfree(thread);
}

void __init thread0_init(void) {
    thread_ctrl_blk_t* thread0 = kmalloc(sizeof(thread_ctrl_blk_t));
    if (unlikely(!thread0))
        return;

    u32 esp;
    __asm__ volatile ("mov %%esp, %0" : "=r"(esp));

    task_ctrl_blk_t* task0 = task_lookup(0);
    
    *thread0 = (thread_ctrl_blk_t) {
        .esp0     = (virt_addr_t)kernel_stack_top,
        .esp      = (virt_addr_t)esp,
        .cr3      = task0->cr3,
        .tid      = 0,
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
