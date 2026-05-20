#include <stddef.h>

#include "kernel/ipc.h"
#include "kernel/panic.h"
#include "kernel/sched.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "lib/list.h"
#include "lib/string.h"
#include "mm/kheap.h"

thread_ctrl_blk_t* cur_running_thread;

static thread_ctrl_blk_t* threads[THREAD_MAX_CNT];

extern task_ctrl_blk_t* ktask;
extern u8               gdt_start[];
extern u8               gdt_kernel_code_seg_desc[];
extern u8               kstack_top[];
extern void             thread_entry_trampoline(void);

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
    thread_get_self()->entry();
    thread_exit();
}

static u32* kstack_init(u32* top) {
    u32* esp = top;
    *(--esp) = 0x202;                                       /* EFLAGS             */
    *(--esp) = (u32)(gdt_kernel_code_seg_desc - gdt_start); /* CS                 */
    *(--esp) = (u32)entry_stub;                             /* EIP                */
    *(--esp) = (u32)thread_entry_trampoline;                /* Thread entry point */
    *(--esp) = 0;                                           /* EBX                */
    *(--esp) = 0;                                           /* ESI                */
    *(--esp) = 0;                                           /* EDI                */
    *(--esp) = 0;                                           /* EBP                */
    return esp;
}

thread_ctrl_blk_t* thread_lookup(u32 tid) {
    return (unlikely(tid >= THREAD_MAX_CNT)) ? NULL : threads[tid];
}

thread_ctrl_blk_t* thread_get_self(void) {
    return cur_running_thread;
}

void __noreturn thread_exit(void) {
    __asm__ volatile ("cli");
    thread_ctrl_blk_t* cur = thread_get_self();
    threads[cur->tid] = NULL;
    sched_zombify();
    __builtin_unreachable();
}

thread_ctrl_blk_t* thread_create(task_ctrl_blk_t* task, thread_entry_func_t entry, u8 priority) {
    if (unlikely(!task || !entry))
        return NULL;

    if (unlikely(priority >= SCHED_PRIORITY_CNT))
        return NULL;

    u8* kstack = (u8*)kmalloc(THREAD_KSTACK_SZ);
    if (unlikely(!kstack))
        return NULL;

    thread_ctrl_blk_t* thread = (thread_ctrl_blk_t*)kmalloc(sizeof(thread_ctrl_blk_t));
    if (unlikely(!thread)) {
        kfree(kstack);
        return NULL;
    }

    memset(thread, 0, sizeof(thread_ctrl_blk_t));
    list_init(&thread->run_link);
    list_init(&thread->wait_link);
    list_init(&thread->task_link);

    thread->reply_port = port_create(1);
    if (unlikely(!thread->reply_port)) {
        kfree(thread);
        kfree(kstack);
        return NULL;
    }

    u32* esp = kstack_init((u32*)(kstack + THREAD_KSTACK_SZ));
    *thread = (thread_ctrl_blk_t) {
        .tid      = 0,
        .esp0     = (virt_addr_t)(kstack + THREAD_KSTACK_SZ),
        .esp      = (virt_addr_t)esp,
        .cr3      = task->cr3,
        .task     = task,
        .state    = THREAD_STATE_READY,
        .priority = priority,
        .quantum  = SCHED_QUANTUM,
        .entry    = entry,
        .kstack   = kstack
    };

    ENTER_CRIT_SEC(flags);
    if (unlikely(alloc_thread_id(thread) < 0)) {
        EXIT_CRIT_SEC(flags);
        port_destroy(thread->reply_port);
        kfree(thread);
        kfree(kstack);
        return NULL;
    }

    list_add_to_tail(&thread->task_link, &task->threads);
    task->nthreads++;

    sched_ready(thread);
    EXIT_CRIT_SEC(flags);
    return thread;
}

void thread_destroy(thread_ctrl_blk_t* thread) {
    if (list_is_attached(&thread->wait_link))
        list_del(&thread->wait_link);

    if (list_is_attached(&thread->task_link))
        list_del(&thread->task_link);

    if (thread->task)
        thread->task->nthreads--;
    
    if (thread->reply_port)
        port_destroy(thread->reply_port);

    if (thread->kstack)
        kfree(thread->kstack);

    kfree(thread);
}

void __init kthread_init(void) {
    thread_ctrl_blk_t* kthread = kmalloc(sizeof(thread_ctrl_blk_t));
    if (unlikely(!kthread))
        return;

    u32 esp;
    __asm__ volatile ("mov %%esp, %0" : "=r"(esp));

    *kthread = (thread_ctrl_blk_t) {
        .esp0     = (virt_addr_t)kstack_top,
        .esp      = (virt_addr_t)esp,
        .cr3      = ktask->cr3,
        .tid      = 0,
        .task     = ktask,
        .state    = THREAD_STATE_RUNNING,
        .priority = SCHED_IDLE_PRIORITY,
        .quantum  = SCHED_QUANTUM
    };

    list_init(&kthread->run_link);
    list_init(&kthread->wait_link);
    list_init(&kthread->task_link);

    threads[0] = kthread;
    list_add_to_tail(&kthread->task_link, &ktask->threads);
    ktask->nthreads++;
    cur_running_thread = kthread;
}
