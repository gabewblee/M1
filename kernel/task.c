#include <stddef.h>

#include "kernel/ipc.h"
#include "kernel/panic.h"
#include "kernel/task.h"
#include "lib/string.h"
#include "mm/kheap.h"

task_ctrl_blk_t* task0;

static task_ctrl_blk_t* tasks[TASK_MAX_CNT];

static task_ctrl_blk_t* task_init(phys_addr_t cr3) {
    task_ctrl_blk_t* task = (task_ctrl_blk_t*)kmalloc(sizeof(task_ctrl_blk_t));
    if (unlikely(!task))
        return NULL;

    *task = (task_ctrl_blk_t) {
        .id   = 0,
        .cr3  = cr3,
        .port = port_create(IPC_QUEUE_CAP)
    };

    if (unlikely(!task->port)) {
        kfree(task);
        return NULL;
    }

    list_init(&task->threads);
    return task;
}

static inline i32 alloc_task_id(task_ctrl_blk_t* task) {
    for (u32 id = 1; id < TASK_MAX_CNT; id++) {
        if (!tasks[id]) {
            tasks[id] = task;
            task->id = id;
            return (i32)id;
        }
    }

    return -1;
}

task_ctrl_blk_t* task_lookup(u32 id) {
    return (unlikely(id >= TASK_MAX_CNT)) ? NULL : tasks[id];
}

task_ctrl_blk_t* task_create(phys_addr_t cr3) {
    task_ctrl_blk_t* task = task_init(cr3);
    if (unlikely(!task))
        return NULL;

    if (unlikely(alloc_task_id(task) < 0)) {
        port_destroy(task->port);
        kfree(task);
        return NULL;
    }
    return task;
}

void task_destroy(const task_ctrl_blk_t* task) {
    if (unlikely(task == task0))
        return;

    if (unlikely(task->nthreads != 0))
        PANIC("Error: Failed to destroy live task");

    tasks[task->id] = NULL;
    port_destroy(task->port);
    kfree((void*)task);
}

void __init task0_init(void) {
    u32 cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    task_ctrl_blk_t* task = task_init(cr3);
    if (unlikely(!task))
        PANIC("Error: Failed to initialize task0");
    
    tasks[0] = task;
    task0 = task;
}
