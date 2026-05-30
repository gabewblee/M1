#pragma once

#include <stddef.h>

#include "config.h"
#include "libk/list.h"

typedef struct ipc_msg_s       ipc_msg_s;
typedef struct port_s          port_s;
typedef struct syscall_frm_s   syscall_frm_s;
typedef struct task_ctrl_blk_s task_ctrl_blk_s;

typedef void (*thread_entry_func_t)(void);

enum thread_state_e {
    THREAD_STATE_RUNNING = 0, /* Thread is running */
    THREAD_STATE_READY   = 1, /* Thread is ready   */
    THREAD_STATE_BLOCKED = 2, /* Thread is blocked */
    THREAD_STATE_ZOMBIE  = 3  /* Thread is zombie  */
};

typedef struct thread_ctrl_blk_s {
    /* Context switch fields */
    virt_addr_t         esp0;         /* Ring 0 stack pointer            */
    virt_addr_t         esp;          /* Stack pointer                   */
    phys_addr_t         cr3;          /* Page directory physical address */

    /* Thread fields         */
    u32                 tid;          /* Thread id                       */
    u8*                 kernel_stack; /* Kernel stack                    */
    thread_entry_func_t entry;        /* Thread entry point              */

    /* Task fields           */
    task_ctrl_blk_s*    task;         /* Owning task                     */
    list_node_s         task_link;    /* Link in owning task list        */
    port_s*             reply_port;   /* Private reply port              */
    const ipc_msg_s*    tx_msg;       /* Pending send message            */
    ipc_msg_s*          rx_msg;       /* Pending receive buffer          */
    
    /* Scheduling fields     */
    u8                  priority;     /* Scheduling priority             */
    u8                  quantum;      /* Remaining time slice            */
    enum thread_state_e state;        /* Scheduler state                 */
    list_node_s         run_link;     /* Link in run queue               */
    list_node_s         wait_link;    /* Link in wait queue              */

    /* Syscall fields        */
    syscall_frm_s*      frm;          /* Current syscall frame           */
} thread_ctrl_blk_s;

/* Assembly offsets for context switching */
_Static_assert(offsetof(thread_ctrl_blk_s, esp0) == 0, "Error: Invalid esp0 offset");
_Static_assert(offsetof(thread_ctrl_blk_s, esp) == 4, "Error: Invalid esp offset");
_Static_assert(offsetof(thread_ctrl_blk_s, cr3) == 8, "Error: Invalid cr3 offset");

/**
 * thread_lookup - Looks up the thread control block associated with @tid.
 * @tid: The thread ID to lookup.
 * Returns: The matching thread control block, or NULL if @tid is out of range.
 */
thread_ctrl_blk_s* thread_lookup(u32 tid);

/**
 * thread_self - Returns the running thread's thread control block.
 * Returns: The running thread's thread control block.
 */
thread_ctrl_blk_s* thread_self(void);

/**
 * thread_exit - Terminates the running thread. Frees its ID and zombifies it.
 *               Never returns.
 */
void __noreturn thread_exit(void);

/**
 * kernel_thread_create - Creates a new kernel thread in @task. Allocates its 
 *                        thread control block, its kernel stack, initializes
 *                        its queues, creates its private reply port, and hands
 *                        it a unique ID. Enqueues it into the run queue, possibly
 *                        preempting the caller if @priority outranks it.
 * @task: The thread's owner task.
 * @entry: The thread's entry function.
 * @priority: The thread's initial scheduling priority.
 * Returns: The new thread control block, or NULL on failure.
 */
thread_ctrl_blk_s* kernel_thread_create(task_ctrl_blk_s* task, thread_entry_func_t entry, const u8 priority);

/**
 * user_thread_create - Creates a new user thread in @task. Allocates its 
 *                      thread control block, its kernel stack, initializes
 *                      its queues, creates its private reply port, and hands
 *                      it a unique ID. Enqueues it into the run queue, possibly
 *                      preempting the caller if @priority outranks it.
 * @task: The thread's owner task.
 * @entry: The thread's entry point virtual address.
 * @user_stack_top: The thread's initial user-mode ESP.
 * @eflags: The thread's initial EFLAGS, including the desired IOPL.
 * @priority: The thread's scheduling priority.
 * Returns: The new thread control block, or NULL on failure.
 */
thread_ctrl_blk_s* user_thread_create(task_ctrl_blk_s* task,
                                      virt_addr_t      entry,
                                      virt_addr_t      user_stack_top,
                                      u32              eflags,
                                      const u8         priority);

/**
 * thread_destroy - Destroys @thread. Detaches itself from its queues,
 *                  decrements the thread count in its task, frees its reply
 *                  port, its kernel stack, and its thread control block.
 * @thread: The thread to destroy.
 *
 * Preconditions:
 * - @thread is not thread0.
 */
void thread_destroy(thread_ctrl_blk_s* thread);

/**
 * thread0_init - Initializes the threading subsystem. Promotes the current
 *                execution context as thread0 by assigning it the current 
 *                kernel stack, task0's address space, handing it ID 0, and
 *                initializing its queues.
 *
 * Preconditions:
 * - The current execution context is thread0.
 */
void __init thread0_init(void);
