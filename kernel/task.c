#include "idt.h"
#include "panic.h"
#include "task.h"

#include "../mm/kheap.h"

#define KERNEL_STACK_SZ 8192

task_ctrl_blk_t* cur_running_task;

static task_queue_t ready;
static task_queue_t zombies;

extern u8   gdt_start[];
extern u8   gdt_kernel_code_seg_desc[];
extern u8   kernel_stack_top[];                
extern void task_switch(task_ctrl_blk_t* nxt); 
extern void task_entry_trampoline(void);       

static void sched_enqueue(task_queue_t* queue, task_ctrl_blk_t* task) {
    task->nxt = NULL;
    if (!queue->head) {
        queue->head = task;
        queue->tail = task;
    } else {
        queue->tail->nxt = task;
        queue->tail      = task;
    }
}

static task_ctrl_blk_t* sched_dequeue(task_queue_t* queue) {
    if (!queue->head)
        return NULL;

    task_ctrl_blk_t* task = queue->head;
    queue->head = task->nxt;
    if (!queue->head)
        queue->tail = NULL;

    task->nxt = NULL;
    return task;
}

static void sched_reap_zombies(void) {
    task_ctrl_blk_t* zombie;
    while ((zombie = sched_dequeue(&zombies))) {
        kfree(zombie->kernel_stack);
        kfree(zombie);
    }
}

static void task_entry_stub(void) {
    cur_running_task->entry();
    task_cur_exit();
}

static u32* task_kernel_stack_init(u32* esp) {
    *(--esp) = 0x202;                                       /* EFLAGS */
    *(--esp) = (u32)(gdt_kernel_code_seg_desc - gdt_start); /* CS */
    *(--esp) = (u32)task_entry_stub;                        /* EIP */
    *(--esp) = (u32)task_entry_trampoline;                  /* Task entry trampoline */
    *(--esp) = 0;                                           /* EBX */
    *(--esp) = 0;                                           /* ESI */
    *(--esp) = 0;                                           /* EDI */
    *(--esp) = 0;                                           /* EBP */
    return esp;
}

static inline u32 task_get_cr3(void) {
    u32 cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    return cr3;
}

static inline u32 task_get_esp(void) {
    u32 esp;
    __asm__ volatile ("mov %%esp, %0" : "=r" (esp));
    return esp;
}

void __hot sched_tick(void) {
    sched_reap_zombies();
    if (!cur_running_task)
        return;

    if (cur_running_task->state == TASK_STATE_RUNNING) {
        cur_running_task->state = TASK_STATE_READY;
        sched_enqueue(&ready, cur_running_task);
    }

    task_ctrl_blk_t* nxt = sched_dequeue(&ready);
    if (!nxt)
        return;

    nxt->state = TASK_STATE_RUNNING;
    if (cur_running_task != nxt)
        task_switch(nxt);
}

void __hot __noreturn task_cur_exit(void) {
    __asm__ volatile ("cli");

    cur_running_task->state = TASK_STATE_ZOMBIE;
    sched_enqueue(&zombies, cur_running_task);

    task_ctrl_blk_t* nxt = sched_dequeue(&ready);
    if (!nxt)
        PANIC("Error: No tasks available after task exit");

    nxt->state = TASK_STATE_RUNNING;
    task_switch(nxt);
    __builtin_unreachable();
}

u32 __hot task_init(task_entry_func_t entry) {
    u8* kernel_stack = (u8*)kmalloc(KERNEL_STACK_SZ);
    if (!kernel_stack)
        return 0;

    task_ctrl_blk_t* task = (task_ctrl_blk_t*)kmalloc(sizeof(task_ctrl_blk_t));
    if (!task) {
        kfree(kernel_stack);
        return 0;
    }

    u32* esp = task_kernel_stack_init((u32*)(kernel_stack + KERNEL_STACK_SZ));

    task->esp0         = (virt_addr_t)(kernel_stack + KERNEL_STACK_SZ);
    task->esp          = (virt_addr_t)esp;
    task->cr3          = task_get_cr3();
    task->nxt          = NULL;
    task->state        = TASK_STATE_READY;
    task->entry        = entry;
    task->kernel_stack = kernel_stack;

    sched_enqueue(&ready, task);
    return 1;
}

void __init multitasking_init(void) {
    task_ctrl_blk_t* task = (task_ctrl_blk_t*)kmalloc(sizeof(task_ctrl_blk_t));
    if (unlikely(!task))
        PANIC("Error: Failed to allocate memory for task control block");

    task->esp0         = (u32)kernel_stack_top;
    task->esp          = task_get_esp();
    task->cr3          = task_get_cr3();
    task->nxt          = NULL;
    task->state        = TASK_STATE_RUNNING;
    task->entry        = NULL;
    task->kernel_stack = NULL;

    cur_running_task = task;
}