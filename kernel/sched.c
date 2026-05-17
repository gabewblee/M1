#include <stddef.h>

#include "idt.h"
#include "panic.h"
#include "sched.h"
#include "thread.h"

#include "../lib/list.h"

static list_node_t run_queue[SCHED_PRIO_CNT];
static list_node_t zombies;
static u32         bitmap;

extern thread_ctrl_blk_t* cur_running_thread;
extern void thread_switch(thread_ctrl_blk_t* nxt);

static inline void run_queue_add(thread_ctrl_blk_t* thread) {
    list_add_to_tail(&thread->run_link, &run_queue[thread->priority]);
    bitmap |= (1u << thread->priority);
}

static inline void run_queue_del(thread_ctrl_blk_t* thread) {
    list_del(&thread->run_link);
    if (list_is_empty(&run_queue[thread->priority]))
        bitmap &= ~(1u << thread->priority);
}

static void schedule(void) {
    if (bitmap == 0) {
        if (unlikely(cur_running_thread->state != THREAD_STATE_RUNNING))
            PANIC("Error: No runnable threads");
        return;
    }

    u32 priority = __builtin_ctz(bitmap);
    thread_ctrl_blk_t* nxt = list_first_entry(
        &run_queue[priority], thread_ctrl_blk_t, run_link
    );

    run_queue_del(nxt);
    nxt->state = THREAD_STATE_RUNNING;
    if (likely(nxt != cur_running_thread))
        thread_switch(nxt);
}

void sched_ready(thread_ctrl_blk_t* thread) {
    thread->state = THREAD_STATE_READY;
    run_queue_add(thread);
    if (thread->priority < cur_running_thread->priority) {
        cur_running_thread->state = THREAD_STATE_READY;
        run_queue_add(cur_running_thread);
        schedule();
    }
}

void sched_block(list_node_t* wait_queue, thread_state_t state) {
    cur_running_thread->state = state;
    list_add_to_tail(&cur_running_thread->run_link, wait_queue);
    schedule();
}

void sched_unblock(thread_ctrl_blk_t* thread) {
    list_del(&thread->run_link);
    sched_ready(thread);
}

void sched_yield(void) {
    enter_crit_sec(flags);
    cur_running_thread->state   = THREAD_STATE_READY;
    cur_running_thread->quantum = SCHED_QUANTUM;
    run_queue_add(cur_running_thread);
    schedule();
    exit_crit_sec(flags);
}

void sched_tick(void) {
    sched_reap();

    if (unlikely(cur_running_thread->state != THREAD_STATE_RUNNING))
        return;

    if (--cur_running_thread->quantum > 0)
        return;

    cur_running_thread->state   = THREAD_STATE_READY;
    cur_running_thread->quantum = SCHED_QUANTUM;
    run_queue_add(cur_running_thread);
    schedule();
}

void __noreturn sched_zombify(void) {
    cur_running_thread->state = THREAD_STATE_ZOMBIE;
    list_add_to_tail(&cur_running_thread->run_link, &zombies);
    schedule();
    __builtin_unreachable();
}

void sched_reap(void) {
    while (!list_is_empty(&zombies)) {
        thread_ctrl_blk_t* zombie = list_first_entry(
            &zombies, thread_ctrl_blk_t, run_link
        );

        list_del(&zombie->run_link);
        thread_destroy(zombie);
    }
}

void __init sched_init(void) {
    for (u32 priority = 0; priority < SCHED_PRIO_CNT; priority++)
        list_init(&run_queue[priority]);

    bitmap = 0;
    list_init(&zombies);
}
