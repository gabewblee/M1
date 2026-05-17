#include <stddef.h>

#include "ipc.h"
#include "panic.h"
#include "task.h"
#include "thread.h"

#include "../lib/string.h"
#include "../mm/kheap.h"

#define _MAX_NUM_TASKS 64

task_ctrl_blk_t* ktask;

static task_ctrl_blk_t* tasks[_MAX_NUM_TASKS];

static inline u32 get_cur_cr3(void) {
    u32 cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline i32 alloc_task_slot(task_ctrl_blk_t* task) {
    for (u32 id = 1; id < _MAX_NUM_TASKS; id++) {
        if (!tasks[id]) {
            tasks[id] = task;
            task->id = id;
            return (i32)id;
        }
    }
    return -1;
}

task_ctrl_blk_t* task_lookup(u32 id) {
    return (unlikely(id >= _MAX_NUM_TASKS)) ? NULL : tasks[id];
}

task_ctrl_blk_t* task_create(phys_addr_t cr3) {
    task_ctrl_blk_t* task = (task_ctrl_blk_t*)kmalloc(sizeof(*task));
    if (unlikely(!task))
        return NULL;

    *task = (task_ctrl_blk_t) {
        .id   = 0,
        .cr3  = cr3,
        .port = port_create(IPC_DEF_CAPACITY)
    };

    if (unlikely(!task->port)) {
        kfree(task);
        return NULL;
    }

    list_init(&task->threads);
    if (unlikely(alloc_task_slot(task) < 0)) {
        port_destroy(task->port);
        kfree(task);
        return NULL;
    }
    return task;
}

void task_destroy(task_ctrl_blk_t* task) {
    if (unlikely(!task || task == ktask))
        return;

    if (unlikely(task->nthreads != 0))
        PANIC("Error: Task still has live threads");

    tasks[task->id] = NULL;
    port_destroy(task->port);
    kfree(task);
}

void __init task_init(void) {
    task_ctrl_blk_t* task = kmalloc(sizeof(task_ctrl_blk_t));
    if (unlikely(!task))
        return;

    *task = (task_ctrl_blk_t) {
        .id   = 0,
        .cr3  = get_cur_cr3(),
        .port = port_create(IPC_DEF_CAPACITY)
    };

    if (unlikely(!task->port)) {
        kfree(task);
        return;
    }

    list_init(&task->threads);
    tasks[0] = task;
    ktask = task;
}
