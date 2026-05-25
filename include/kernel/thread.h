#pragma once

#include <stddef.h>

#include "config.h"
#include "lib/list.h"

typedef struct ipc_msg_t ipc_msg_t;
typedef struct port_t port_t;
typedef struct syscall_frm_t syscall_frm_t;
typedef struct task_ctrl_blk_t task_ctrl_blk_t;
typedef void   (*thread_entry_func_t)(void);

typedef enum thread_state_t {
    THREAD_STATE_RUNNING = 0, /* Thread is running */
    THREAD_STATE_READY   = 1, /* Thread is ready   */
    THREAD_STATE_BLOCKED = 2, /* Thread is blocked */
    THREAD_STATE_ZOMBIE  = 3  /* Thread is zombie  */
} thread_state_t;

typedef struct thread_ctrl_blk_t {
    /* Context switch fields */
    virt_addr_t         esp0;         /* Ring 0 stack pointer            */
    virt_addr_t         esp;          /* Stack pointer                   */
    phys_addr_t         cr3;          /* Page directory physical address */

    /* Thread fields         */
    u32                 tid;          /* Thread id                       */
    u8*                 kernel_stack; /* Kernel stack                    */
    thread_entry_func_t entry;        /* Thread entry point              */

    /* Task fields           */
    task_ctrl_blk_t*    task;         /* Owning task                     */
    list_node_t         task_link;    /* Link in owning task list        */
    port_t*             reply_port;   /* Private reply port              */
    const ipc_msg_t*    tx_msg;       /* Pending send message            */
    ipc_msg_t*          rx_msg;       /* Pending receive buffer          */
    
    /* Scheduling fields     */
    u8                  priority;     /* Scheduling priority             */
    u8                  quantum;      /* Remaining time slice            */
    thread_state_t      state;        /* Scheduler state                 */
    list_node_t         run_link;     /* Link in run queue               */
    list_node_t         wait_link;    /* Link in wait queue              */

    /* Syscall fields        */
    syscall_frm_t*      frm;          /* Current syscall frame           */
} thread_ctrl_blk_t;

/* Assembly offsets for context switching */
_Static_assert(offsetof(thread_ctrl_blk_t, esp0) == 0, "Error: esp0 is not at offset 0");
_Static_assert(offsetof(thread_ctrl_blk_t, esp) == 4, "Error: esp is not at offset 4");
_Static_assert(offsetof(thread_ctrl_blk_t, cr3) == 8, "Error: cr3 is not at offset 8");

/**
 * thread_lookup - Retrieves the thread control block associated with @tid.
 * @tid: The thread ID to lookup.
 * Returns: The matching thread control block, or NULL if @tid is out of range.
 */
thread_ctrl_blk_t* thread_lookup(u32 tid);

/**
 * thread_self - Retrieves the running thread's thread control block.
 * Returns: The running thread's thread control block.
 */
thread_ctrl_blk_t* thread_self(void);

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
thread_ctrl_blk_t* kernel_thread_create(task_ctrl_blk_t* task, thread_entry_func_t entry, u8 priority) __attribute__((warn_unused_result));

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
thread_ctrl_blk_t* user_thread_create(task_ctrl_blk_t* task,
                                      virt_addr_t      entry,
                                      virt_addr_t      user_stack_top,
                                      u32              eflags,
                                      u8               priority);

/**
 * thread_destroy - Destroys @thread. Detaches itself from its queues,
 *                  decrements the thread count in its task, frees its reply
 *                  port, its kernel stack, and its thread control block.
 * @thread: The thread to destroy.
 *
 * NOTE: The caller must ensure that @thread is not thread0.
 */
void thread_destroy(thread_ctrl_blk_t* thread);

/**
 * thread0_init - Initializes the threading subsystem. Promotes the current
 *                execution context as thread0 by assigning it the current 
 *                kernel stack, task0's address space, handing it ID 0, and
 *                initializing its queues.
 *
 * NOTE: Thread0 is undestroyable.
 */
void __init thread0_init(void);
