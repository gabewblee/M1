#include <kernel/core/initcall.h>
#include <kernel/core/panic.h>
#include <kernel/core/task.h>
#include <libk/string.h>
#include <mm/kheap.h>
#include <stddef.h>

task_ctrl_blk_s* task0;

static task_ctrl_blk_s* tasks[TASK_MAX_CNT];

static task_ctrl_blk_s* task_init(phys_addr_t cr3) {
    task_ctrl_blk_s* task = (task_ctrl_blk_s*)kmalloc(sizeof(task_ctrl_blk_s));
    if (unlikely(!task))
        return NULL;

    *task = (task_ctrl_blk_s){
        .id  = 0,
        .cr3 = cr3,
    };

    list_init(&task->threads);
    return task;
}

static inline i32 alloc_task_id(task_ctrl_blk_s* task) {
    for (u32 id = 1; id < TASK_MAX_CNT; id++) {
        if (!tasks[id]) {
            tasks[id] = task;
            task->id = id;
            return (i32)id;
        }
    }

    return -1;
}

task_ctrl_blk_s* task_lookup(u32 id) {
    return (unlikely(id >= TASK_MAX_CNT)) ? NULL : tasks[id];
}

task_ctrl_blk_s* task_create(phys_addr_t cr3) {
    task_ctrl_blk_s* task = task_init(cr3);
    if (unlikely(!task))
        return NULL;

    if (unlikely(alloc_task_id(task) < 0)) {
        kfree(task);
        return NULL;
    }
    
    return task;
}

void task_destroy(task_ctrl_blk_s* task) {
    if (unlikely(task == task0))
        return;

    if (unlikely(task->nthreads != 0))
        PANIC("Error: Failed to destroy task with live threads");

    tasks[task->id] = NULL;
    kfree((void*)task);
}

void __init task0_init(void) {
    u32 cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    task_ctrl_blk_s* task = task_init(cr3);
    if (unlikely(!task))
        PANIC("Error: Failed to initialize task0");
    
    tasks[0] = task;
    task0 = task;
}

user_initcall(task0_init);
