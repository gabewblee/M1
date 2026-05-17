#pragma once

#include "ipc.h"

#include "../config.h"
#include "../lib/list.h"

typedef struct task_ctrl_blk_t {
    u32         id;
    phys_addr_t cr3;
    list_node_t threads;
    u32         nthreads;
    port_t*     port;
} task_ctrl_blk_t;

/**
 * task_lookup - Resolves a task by id.
 * @id: The id of the task to lookup.
 * @return: The task, or NULL on failure.
 */
task_ctrl_blk_t* task_lookup(u32 id);

/**
 * task_create - Allocates a new task with the given address space and an
 *               empty thread list. The task starts with one task port whose
 *               queue is sized by the IPC defaults.
 * @cr3: The physical address of the page directory backing the address space.
 * @return: The new task, or NULL on failure.
 */
task_ctrl_blk_t* task_create(phys_addr_t cr3);

/**
 * task_destroy - Frees the task's resources. The caller must ensure no 
 *                threads remain.
 * @task: The task to destroy.
 */
void task_destroy(task_ctrl_blk_t* task);

/**
 * task_init - Initializes multitasking and creates the kernel task (tid 0).
 */
void __init task_init(void);
