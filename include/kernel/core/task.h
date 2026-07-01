#pragma once

#include <config.h>
#include <libk/list.h>

#define KERNEL_TASK_ID 0u

typedef struct task_ctrl_blk_s {
    /* Task fields   */
    u32         id;       /* Task ID                         */
    phys_addr_t cr3;      /* Page directory physical address */

    /* Thread fields */
    u32         nthreads; /* Thread list count               */
    list_node_s threads;  /* Thread list head                */
} task_ctrl_blk_s;

/**
 * task_lookup - Looks up the task control block associated with @id.
 * @id: The task ID to lookup.
 * Returns: The matching task control block, or NULL on failure.
 */
task_ctrl_blk_s* task_lookup(u32 id);

/**
 * task_create - Creates a new task with address space @cr3.
 * @cr3: The physical address of the page directory for the new task.
 * Returns: The new task control block, or NULL on failure.
 */
task_ctrl_blk_s* task_create(phys_addr_t cr3);

/**
 * task_destroy - Destroys @task.
 * @task: The task to destroy.
 *
 * Preconditions:
 * - @task is not task0.
 * - @task has no live threads.
 */
void task_destroy(task_ctrl_blk_s* task);

/**
 * task0_init - Initializes the tasking subsystem.
 */
void __init task0_init(void);
