#pragma once

#include "config.h"
#include "kernel/core/thread.h"
#include "arch/x86/idt.h"
#include "libk/list.h"

#define SCHED_PRIORITY_CNT  32                       /* Priority count            */
#define SCHED_IDLE_PRIORITY (SCHED_PRIORITY_CNT - 1) /* Idle scheduling priority  */
#define SCHED_QUANTUM       4                        /* Time slice in timer ticks */

/**
 * sched_pit_handler - Handles scheduling activities per timer tick.
 * @frm: The pointer to the interrupt stack frame.
 */
void sched_pit_handler(const int_frm_s* frm);

/**
 * sched_ready - Prepares @thread for scheduling.
 * @thread: The thread to enqueue.
 */
void sched_ready(thread_ctrl_blk_s* thread);

/**
 * sched_block - Blocks the running thread on @wait_queue with state @state.
 * @wait_queue: The wait queue to block on.
 * @state: The blocked state to assign.
 */
void sched_block(list_node_s* wait_queue, thread_state_e state);

/**
 * sched_unblock - Unblocks @thread.
 * @thread: The thread to wake.
 */
void sched_unblock(thread_ctrl_blk_s* thread);

/**
 * sched_yield - Yields the CPU to the highest priority runnable thread.
 */
void sched_yield(void);

/**
 * sched_tick - Handles scheduling activities per timer tick.
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
 * sched_init - Initializes the scheduling subsystem.
 */
void __init sched_init(void);
