#pragma once

#include <arch/x86/idt.h>
#include <config.h>
#include <kernel/core/thread.h>
#include <kernel/sync/spinlock.h>
#include <libk/list.h>

#define SCHED_PRIORITY_CNT  32                       /* Priority count            */
#define SCHED_IDLE_PRIORITY (SCHED_PRIORITY_CNT - 1) /* Idle scheduling priority  */
#define SCHED_QUANTUM       4                        /* Time slice in timer ticks */

/**
 * sched_pit_handler - Handles PIT interrupts.
 * @frm: The pointer to the interrupt stack frame.
 */
void sched_pit_handler(ifrm_s* frm);

/**
 * sched_ready - Prepares @thread for scheduling. Preempts the running thread if @thread 
 *               has a higher priority.
 * @thread: The thread to prepare.
 */
void sched_ready(thread_ctrl_blk_s* thread);

/**
 * sched_enqueue - Prepares @thread for scheduling. Does not preempt the running thread
 *                 if @thread has a higher priority.
 * @thread: The thread to enqueue.
 */
void sched_enqueue(thread_ctrl_blk_s* thread);

/**
 * sched_block - Blocks the running thread on @wait_queue with @state, then reschedules.
 * @wait_queue: The wait queue to block on.
 * @state: The blocked state to assign.
 */
void sched_block(list_node_s* wait_queue, thread_state_e state);

/**
 * sched_block_and_release - Enqueues the running thread on @wait_queue with @state
 *                           while @lock is held, releases @lock, then reschedules.
 * @wait_queue: The wait queue to block on.
 * @state: The blocked state to assign.
 * @lock: The lock guarding @wait_queue, released before rescheduling.
 * @flags: The EFLAGS returned by spin_lock_irqsave when @lock was acquired.
 */
void sched_block_and_release(list_node_s* wait_queue, thread_state_e state, spinlock_s* lock, u32 flags);

/**
 * sched_unblock - Unblocks @thread, then prepares it for scheduling.
 * @thread: The thread to unblock.
 */
void sched_unblock(thread_ctrl_blk_s* thread);

/**
 * sched_yield - Yields to the highest priority thread.
 */
void sched_yield(void);

/**
 * sched_zombify - Zombifies the running thread, then reschedules.
 */
void __noreturn sched_zombify(void);

/**
 * sched_reap_zombies - Reaps zombie threads.
 */
void sched_reap_zombies(void);
