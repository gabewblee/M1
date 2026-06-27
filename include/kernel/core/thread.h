#pragma once

#include <config.h>
#include <libk/list.h>
#include <stddef.h>

typedef struct ipc_packet_s    ipc_packet_s;
typedef struct task_port_s     task_port_s;
typedef struct task_ctrl_blk_s task_ctrl_blk_s;

typedef enum thread_state_e {
    THREAD_STATE_RUNNING = 0, /* Thread is running */
    THREAD_STATE_READY   = 1, /* Thread is ready   */
    THREAD_STATE_BLOCKED = 2, /* Thread is blocked */
    THREAD_STATE_ZOMBIE  = 3  /* Thread is zombie  */
} thread_state_e;

typedef struct thread_ctrl_blk_s {
    /* Context switch fields */
    virt_addr_t      esp0;       /* Ring 0 stack pointer            */
    virt_addr_t      esp;        /* Ring 3 stack pointer            */
    phys_addr_t      cr3;        /* Page directory physical address */

    /* Thread fields         */
    u32              id;         /* Thread id                       */
    u8*              stack0;     /* Kernel stack                    */

    /* Task fields           */
    task_ctrl_blk_s* task;       /* Owning task                     */
    list_node_s      task_link;  /* Link in owning task list        */
    task_port_s*     reply_port; /* Private reply port              */
    ipc_packet_s*    tx_packet;  /* Pending send packet             */
    ipc_packet_s*    rx_packet;  /* Pending receive buffer          */
    
    /* Scheduling fields     */
    u8               priority;   /* Scheduling priority             */
    u8               quantum;    /* Remaining time slice            */
    thread_state_e   state;      /* Scheduler state                 */
    list_node_s      run_link;   /* Link in run queue               */
    list_node_s      wait_link;  /* Link in wait queue              */
} thread_ctrl_blk_s;

/* Assembly offsets for context switching */
_Static_assert(offsetof(thread_ctrl_blk_s, esp0) == 0, "Error: esp0 offset within thread_ctrl_blk_s must be 0");
_Static_assert(offsetof(thread_ctrl_blk_s, esp)  == 4, "Error: esp offset within thread_ctrl_blk_s must be 4");
_Static_assert(offsetof(thread_ctrl_blk_s, cr3)  == 8, "Error: cr3 offset within thread_ctrl_blk_s must be 8");

/**
 * thread_lookup - Looks up the thread control block associated with @id.
 * @id: The thread ID to lookup.
 * Returns: The matching thread control block, or NULL on failure.
 */
thread_ctrl_blk_s* thread_lookup(u32 id);

/**
 * thread_self - Fetches the running thread's thread control block.
 * Returns: The running thread's thread control block.
 */
thread_ctrl_blk_s* thread_self(void);

/**
 * thread_exit - Terminates the running thread.
 */
void __noreturn thread_exit(void);

/**
 * kernel_thread_create - Creates a new kernel thread in @task.
 * @task: The thread's owner task.
 * @entry: The thread's initial entry point virtual address.
 * @priority: The thread's initial scheduling priority.
 * Returns: The new thread control block, or NULL on failure.
 */
thread_ctrl_blk_s* kernel_thread_create(task_ctrl_blk_s* task, virt_addr_t entry, u8 priority);

/**
 * user_thread_create - Creates a new user thread in @task.
 * @task: The thread's owner task.
 * @entry: The thread's initial entry point virtual address.
 * @user_stack_top: The thread's initial user-mode ESP.
 * @eflags: The thread's initial EFLAGS, including the desired IOPL.
 * @priority: The thread's initial scheduling priority.
 * Returns: The new thread control block, or NULL on failure.
 */
thread_ctrl_blk_s* user_thread_create(task_ctrl_blk_s* task,
                                      virt_addr_t      entry,
                                      virt_addr_t      user_stack_top,
                                      u32              eflags,
                                      u8               priority);

/**
 * thread_destroy - Destroys @thread.
 * @thread: The thread to destroy.
 *
 * Preconditions:
 * - @thread is not thread0.
 */
void thread_destroy(thread_ctrl_blk_s* thread);

/**
 * thread0_init - Initializes the threading subsystem.
 */
void __init thread0_init(void);
