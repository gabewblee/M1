#pragma once

#include "config.h"
#include "lib/list.h"

typedef struct port_t port_t;

typedef struct task_ctrl_blk_t {
    u32         id;       /* Task id                         */
    u32         nthreads; /* Thread list count               */
    list_node_t threads;  /* Thread list head                */
    phys_addr_t cr3;      /* Page directory physical address */
    port_t*     port;     /* Task port                       */
} task_ctrl_blk_t;

/**
 * task_lookup - Finds a task control block by task id.
 * @id: The task id to resolve.
 * @return: The matching task control block, or NULL on failure.
 */
task_ctrl_blk_t* task_lookup(u32 id);

/**
 * task_create - Creates a task record for an address space with 
 *               an empty thread list and a default task port.
 * @cr3: The physical address of the page directory for the new task.
 * @return: The new task control block, or NULL on failure.
 */
task_ctrl_blk_t* task_create(phys_addr_t cr3);

/**
 * task_destroy - Releases a task and its task-level resources. 
 *                Caller must guarantee the task has no remaining
 *                threads.
 * @task: The task to destroy.
 */
void task_destroy(task_ctrl_blk_t* task);

/**
 * ktask_init - Initializes the kernel task. 
 */
void __init ktask_init(void);
