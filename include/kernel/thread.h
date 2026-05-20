#pragma once

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
    virt_addr_t         esp0;         /* Ring 0 stack pointer            */
    virt_addr_t         esp;          /* Stack pointer                   */
    phys_addr_t         cr3;          /* Page directory physical address */
    list_node_t         run_link;     /* Link in run queue               */
    list_node_t         wait_link;    /* Link in wait queue              */
    list_node_t         task_link;    /* Link in owning task list        */
    u32                 tid;          /* Thread id                       */
    task_ctrl_blk_t*    task;         /* Owning task                     */
    thread_state_t      state;        /* Scheduler state                 */
    u8                  priority;     /* Scheduling priority             */
    u8                  quantum;      /* Remaining time slice            */
    thread_entry_func_t entry;        /* Thread entry point              */
    u8*                 kernel_stack; /* Kernel stack                    */
    port_t*             reply_port;   /* Private reply port              */
    const ipc_msg_t*    tx_msg;       /* Pending send message            */
    ipc_msg_t*          rx_msg;       /* Pending receive buffer          */
    syscall_frm_t*      frm;          /* Current syscall frame           */
} thread_ctrl_blk_t;

_Static_assert(__builtin_offsetof(thread_ctrl_blk_t, esp0) == 0, "esp0 must be at offset 0");
_Static_assert(__builtin_offsetof(thread_ctrl_blk_t, esp) == 4, "esp must be at offset 4");
_Static_assert(__builtin_offsetof(thread_ctrl_blk_t, cr3) == 8, "cr3 must be at offset 8");

/**
 * thread_lookup - Finds a thread control block by thread id.
 * @tid: The thread id to resolve.
 * @return: The matching thread control block, or NULL on failure.
 */
thread_ctrl_blk_t* thread_lookup(u32 tid);

/**
 * thread_self - Returns the currently running thread.
 * @return: The current thread control block.
 */
thread_ctrl_blk_t* thread_self(void);

/**
 * thread_exit - Terminates the currently running thread.
 */
void __noreturn thread_exit(void);

/**
 * kernel_thread_create - Creates and prepares a new kernel thread in @task.
 * @task: The owning task for the new thread.
 * @entry: The thread entry function.
 * @priority: The initial scheduler priority.
 * @return: The new thread control block, or NULL on failure.
 */
thread_ctrl_blk_t* kernel_thread_create(task_ctrl_blk_t* task, thread_entry_func_t entry, u8 priority);

/**
 * user_thread_create - Creates and prepares a new user thread in @task.
 * @task: The owning task for the new thread.
 * @entry: The user thread entry point virtual address.
 * @user_stack_top: The initial user-mode ESP.
 * @eflags: The initial EFLAGS, including the desired IOPL.
 * @priority: The initial scheduler priority.
 * @return: The new thread control block, or NULL on failure.
 */
thread_ctrl_blk_t* user_thread_create(task_ctrl_blk_t* task,
                                      virt_addr_t      entry,
                                      virt_addr_t      user_stack_top,
                                      u32              eflags,
                                      u8               priority);

/**
 * thread_destroy - Releases resources owned by a thread.
 * @thread: The thread to destroy.
 */
void thread_destroy(thread_ctrl_blk_t* thread);

/**
 * thread0_init - Initializes the bootstrap thread for the
 *                current kernel context.
 */
void __init thread0_init(void);
