#pragma once

#include "config.h"
#include "lib/list.h"

typedef struct port_t port_t;

typedef struct task_ctrl_blk_t {
    u32         id;       /* Task ID                         */
    u32         nthreads; /* Thread list count               */
    list_node_t threads;  /* Thread list head                */
    phys_addr_t cr3;      /* Page directory physical address */
    port_t*     port;     /* IPC port for message passing    */
} task_ctrl_blk_t;

/**
 * task_lookup - Retrieves the task control block associated with @id.
 * @id: The task ID to lookup.
 * Returns: The matching task control block, or NULL if @id is out of range.
 */
task_ctrl_blk_t* task_lookup(u32 id);

/**
 * task_create - Creates a new task control block. Assigns it @cr3 as its
 *               address space, allocates its IPC port, initializes its
 *               thread list, and hands it a unique ID.
 * @cr3: The physical address of the page directory for the new task.
 * Returns: The new task control block, or NULL on failure.
 */
task_ctrl_blk_t* task_create(phys_addr_t cr3);

/**
 * task_destroy - Destroys @task. Frees the its ID, its IPC port, and its
 *                task control block.
 * @task: The task to destroy.
 *
 * NOTE: The caller must ensure that @task is not task0 and that it has
 *       no live threads left.
 */
void task_destroy(const task_ctrl_blk_t* task);

/**
 * task0_init - Initializes tasking subsystem. Promotes the current execution
 *              context as task0 by assigning it the current address space, 
 *              allocating its IPC port, initializing its thread list, and
 *              handing it ID 0.

 * NOTE: Task0 is undestroyable.
 *
 * Context: The task0 is the kernel task, responsible for the management of
 *          kernel resources, handling of system calls, and provision of
 *          kernel services to user-space applications.
 */
void __init task0_init(void);