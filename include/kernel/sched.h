#pragma once

#include "config.h"
#include "kernel/thread.h"
#include "lib/list.h"

#define SCHED_PRIORITY_CNT  32                       /* Priority count             */
#define SCHED_IDLE_PRIORITY (SCHED_PRIORITY_CNT - 1) /* Lowest scheduling priority */
#define SCHED_QUANTUM       4                        /* Time slice                 */

/**
 * sched_ready - Marks a thread runnable and enqueues it by priority. May 
 *               preempt the current thread if @thread has higher scheduling
 *               priority.
 * @thread: The thread to enqueue.
 */
void sched_ready(thread_ctrl_blk_t* thread);

/**
 * sched_block - Blocks the current thread on a wait queue and reschedules.
 *               Caller must have interrupts disabled.
 * @wait_queue: The wait queue to link the current thread into.
 * @state: The blocked state to assign before sleeping.
 */
void sched_block(list_node_t* wait_queue, thread_state_t state);

/**
 * sched_unblock - Removes a blocked thread from its wait queue and readies it.
 *                 May preempt the current thread if @thread outranks it.
 * @thread: The blocked thread to wake.
 */
void sched_unblock(thread_ctrl_blk_t* thread);

/**
 * sched_yield - Voluntarily gives up the CPU to the best runnable thread.
 */
void sched_yield(void);

/**
 * sched_tick - Handles timer tick accounting for the running thread. Intended
 *              to run from the timer IRQ path, trigger preemption on quantum 
 *              expiry, and reap zombie threads opportunistically.
 */
void sched_tick(void);

/**
 * sched_zombify - Marks the current thread as zombie and schedules away.
 *                 Never returns.
 */
void __noreturn sched_zombify(void);

/**
 * sched_reap - Reclaims resources for threads in zombie state.
 */
void sched_reap(void);

/**
 * sched_init - Initializes scheduler queues and internal state.
 */
void __init sched_init(void);
