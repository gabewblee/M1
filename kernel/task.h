#pragma once

#include "../config.h"

typedef enum {
    RUNNING = 0,                          /* Task is running */
    READY   = 1,                          /* Task is ready */
    BLOCKED = 2,                          /* Task is blocked */
    ZOMBIE  = 3                           /* Task is dead */
} task_state_t;

typedef void (*task_entry_func_t)(void);  /* Task entry point */

typedef struct task_ctrl_blk_t {
    u32                     esp0;         /* Kernel stack top */
    u32                     esp;          /* Stack pointer */
    u32                     cr3;          /* Page directory base register */
    struct task_ctrl_blk_t* nxt;          /* Next task in the queue */
    task_state_t            state;        /* Task state */
    task_entry_func_t       entry;        /* Task entry point */
    u8*                     kernel_stack; /* Kernel stack */
} task_ctrl_blk_t;

typedef struct task_queue_t {
    task_ctrl_blk_t* head;                /* Head of the queue */
    task_ctrl_blk_t* tail;                /* Tail of the queue */
} task_queue_t;

/**
 * sched_tick - Perform a scheduling tick
 */
void __hot sched_tick(void);

/**
 * task_init - Initialize a task
 * @entry: The entry point for the task
 */
void __hot task_init(task_entry_func_t entry);

/**
 * multitasking_init - Initialize multitasking
 */
void __init multitasking_init(void);