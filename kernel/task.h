#pragma once

#include "../config.h"

typedef enum task_state_t {
    TASK_STATE_RUNNING = 0,
    TASK_STATE_READY   = 1,
    TASK_STATE_ZOMBIE  = 3
} task_state_t;

typedef void (*task_entry_func_t)(void);

typedef struct task_ctrl_blk_t {
    u32                     esp0;
    u32                     esp;
    u32                     cr3;
    struct task_ctrl_blk_t* nxt;
    task_state_t            state;
    task_entry_func_t       entry;
    u8*                     kernel_stack;
} task_ctrl_blk_t;

typedef struct task_queue_t {
    task_ctrl_blk_t* head;
    task_ctrl_blk_t* tail;
} task_queue_t;

/**
 * sched_tick - Yields the CPU to the next ready task. No-op 
 *              if no other task is ready to run.
 */
void __hot sched_tick(void);

/**
 * task_init - Spawns a new task with the given entry point.
 * @entry: The task entry point.
 * @return: 1 on success, 0 on failure.
 */
u32 __hot task_init(task_entry_func_t entry);

/**
 * task_cur_exit - Terminates the current task and switches 
 *                 to the next ready task.
 */
void __hot __noreturn task_cur_exit(void);

/**
 * multitasking_init - Initializes the multitasking subsystem.
 */
void __init multitasking_init(void);