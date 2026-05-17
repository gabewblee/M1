#pragma once

#include "../config.h"
#include "../lib/list.h"
#include "thread.h"

#define SCHED_PRIO_CNT  32
#define SCHED_PRIO_IDLE (SCHED_PRIO_CNT - 1)
#define SCHED_QUANTUM   4

/**
 * sched_ready - Inserts @thread into the ready queue at its priority. If @thread
 *               outranks the current thread it preempts immediately,
 *               otherwise control returns to the caller.
 * @thread: The thread to enqueue.
 */
void sched_ready(thread_ctrl_blk_t* thread);

/**
 * sched_block - Suspends the running thread on @wait_queue with the given state and
 *               surrenders the CPU. Caller must hold IRQs disabled. Returns
 *               only after sched_unblock() puts the thread back on the queue.
 */
void sched_block(list_node_t* wait_queue, thread_state_t state);

/**
 * sched_unblock - Detaches @thread from whichever wait list it lives on and makes
 *                 it runnable. Preempts the caller if @thread outranks it.
 */
void sched_unblock(thread_ctrl_blk_t* thread);

/**
 * sched_yield - Voluntarily round-robins to the next thread at the same or
 *               higher priority.
 */
void sched_yield(void);

/**
 * sched_tick - Quantum accounting; invoked from the timer IRQ. Reaps zombies
 *              opportunistically and preempts on quantum expiry.
 */
void sched_tick(void);

/**
 * sched_zombify - Marks the running thread as a zombie and schedules away.
 *                 Never returns; the reaper releases the storage.
 */
void __noreturn sched_zombify(void);

/**
 * sched_reap - Drains the zombie list, releasing thread resources. Called
 *              from sched_tick() and any other safe point.
 */
void sched_reap(void);

/**
 * sched_init - Initializes the runqueue and zombie list.
 */
void __init sched_init(void);
