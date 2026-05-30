#pragma once

#include "config.h"
#include "kernel/thread.h"
#include "arch/x86/idt.h"
#include "libk/list.h"

#define SCHED_PRIORITY_CNT  32                       /* Priority count            */
#define SCHED_IDLE_PRIORITY (SCHED_PRIORITY_CNT - 1) /* Idle scheduling priority  */
#define SCHED_QUANTUM       4                        /* Time slice in timer ticks */

/**
 * sched_pit_handler - Handles PIT timer IRQs. Calls sched_tick().
 * @frm: The pointer to the interrupt stack frame.
 */
void sched_pit_handler(const int_frm_s* frm);

/**
 * sched_ready - Prepares @thread for scheduling. Adds @thread to the run queue
 *               and immediately re-schedules if it has a higher priority than the
 *               running thread.
 * @thread: The thread to enqueue.
 */
void sched_ready(thread_ctrl_blk_s* thread);

/**
 * sched_block - Blocks the running thread. Links the thread into @wait_queue, 
 *               sets its state to @state, and schedules away.
 * @wait_queue: The wait queue to link into.
 * @state: The blocked state to assign.
 */
void sched_block(list_node_s* wait_queue, enum thread_state_e state);

/**
 * sched_unblock - Unblocks @thread. Removes @thread from its wait queue and
 *                 prepares it for scheduling. May preempt the running thread
 *                 when @thread has a lower priority value.
 * @thread: The thread to wake.
 */
void sched_unblock(thread_ctrl_blk_s* thread);

/**
 * sched_yield - Yields the CPU to the highest priority runnable thread.
 */
void sched_yield(void);

/**
 * sched_tick - Handles scheduling activities for each timer tick. Reaps
 *              zombie threads and reschedules if the running thread has
 *              expired its time slice.
 */
void sched_tick(void);

/**
 * sched_zombify - Zombifies the current thread, adds it to the zombie list,
 *                 and schedules away.
 */
void __noreturn sched_zombify(void);

/**
 * sched_reap - Reaps all zombie threads.
 */
void sched_reap(void);

/**
 * sched_init - Initializes scheduling subsystem. Registers the PIT timer
 *              IRQ handler.
 */
void __init sched_init(void);
