#pragma once

#include "ipc.h"
#include "task.h"
#include "syscall.h"

#include "../config.h"
#include "../lib/list.h"

typedef struct task_ctrl_blk_t task_ctrl_blk_t;
typedef void (*thread_entry_func_t)(void);

typedef enum thread_state_t {
    THREAD_STATE_RUNNING = 0,
    THREAD_STATE_READY   = 1,
    THREAD_STATE_BLOCKED = 2,
    THREAD_STATE_ZOMBIE  = 3
} thread_state_t;

typedef struct thread_ctrl_blk_t {
    virt_addr_t         esp0;
    virt_addr_t         esp;
    phys_addr_t         cr3;
    list_node_t         run_link;
    list_node_t         task_link;
    u32                 tid;
    task_ctrl_blk_t*    task;
    thread_state_t      state;
    u8                  priority;
    u8                  quantum;
    thread_entry_func_t entry;
    u8*                 kstack;
    port_t*             reply_port;
    const ipc_msg_t*    tx_msg;
    ipc_msg_t*          rx_msg;
    syscall_frm_t*      frm;
} thread_ctrl_blk_t;

_Static_assert(__builtin_offsetof(thread_ctrl_blk_t, esp0) == 0, "esp0 must be at offset 0");
_Static_assert(__builtin_offsetof(thread_ctrl_blk_t, esp) == 4, "esp must be at offset 4");
_Static_assert(__builtin_offsetof(thread_ctrl_blk_t, cr3) == 8, "cr3 must be at offset 8");

/**
 * thread_lookup - Resolves a thread by id.
 * @tid: The id of the thread to lookup.
 * @return: The thread, or NULL on failure.
 */
thread_ctrl_blk_t* thread_lookup(u32 tid);

/**
 * thread_get_self - Get the currently running thread.
 * @return: The currently running thread.
 */
thread_ctrl_blk_t* thread_get_self(void);

/**
 * thread_exit - Terminates the running thread.
 */
void __noreturn thread_exit(void);

/**
 * thread_create - Creates a new thread inside @task that begins execution at @entry with the 
 *                 given scheduling priority. The thread is made runnable before returning 
 *                 and will preempt the caller if its priority is higher.
 * @task: The task to create the thread in.
 * @entry: The entry point of the thread.
 * @priority: The scheduling priority of the thread.
 * @return: The thread, or NULL on failure.
 */
thread_ctrl_blk_t* thread_create(task_ctrl_blk_t* task, thread_entry_func_t entry, u8 priority);

/**
 * thread_destroy - Frees the thread's resources.
 * @thread: The thread to destroy.
 */
void thread_destroy(thread_ctrl_blk_t* thread);

/**
 * thread_init - Promotes the current execution context into the kernel's idle thread (tid 0).
 */
void __init thread_init(void);
