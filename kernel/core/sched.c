#include <arch/x86/idt.h>
#include <kernel/core/initcall.h>
#include <kernel/core/panic.h>
#include <kernel/core/sched.h>
#include <kernel/core/thread.h>
#include <kernel/sync/spinlock.h>
#include <libk/list.h>

static list_node_s run_queue[SCHED_PRIORITY_CNT];
static list_node_s zombies;
static u32         bitmap;

extern void thread_switch(thread_ctrl_blk_s* nxt);

static inline u32 irq_save(void) {
    u32 flags;
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(u32 flags) {
    __asm__ volatile("push %0; popf" : : "r"(flags) : "memory", "cc");
}

static inline void run_queue_add(thread_ctrl_blk_s* thread) {
    list_add_to_tail(&thread->run_link, &run_queue[thread->priority]);
    bitmap |= (1u << thread->priority);
}

static inline void run_queue_del(thread_ctrl_blk_s* thread) {
    list_del(&thread->run_link);
    if (list_is_empty(&run_queue[thread->priority]))
        bitmap &= ~(1u << thread->priority);
}

/* Must be called with local interrupts disabled. */
static void schedule(void) {
    thread_ctrl_blk_s* cur = thread_self();
    if (bitmap == 0) {
        if (unlikely(cur->state != THREAD_STATE_RUNNING))
            PANIC("Error: Failed to schedule thread");

        return;
    }

    u32 priority = __builtin_ctz(bitmap);
    thread_ctrl_blk_s* nxt = list_first_entry(
        &run_queue[priority], thread_ctrl_blk_s, run_link
    );

    run_queue_del(nxt);
    nxt->state = THREAD_STATE_RUNNING;
    if (likely(nxt != cur))
        thread_switch(nxt);
}

void sched_pit_handler(ifrm_s* frm) {
    (void)frm;
    sched_reap_zombies();

    thread_ctrl_blk_s* cur = thread_self();
    if (unlikely(cur->state != THREAD_STATE_RUNNING))
        return;

    cur->quantum--;
    if (cur->quantum > 0)
        return;

    cur->state   = THREAD_STATE_READY;
    cur->quantum = SCHED_QUANTUM;
    run_queue_add(cur);
    schedule();
}

void sched_ready(thread_ctrl_blk_s* thread) {
    u32 flags = irq_save();
    thread_ctrl_blk_s* cur = thread_self();

    thread->state = THREAD_STATE_READY;
    run_queue_add(thread);
    if (thread->priority < cur->priority) {
        cur->state = THREAD_STATE_READY;
        run_queue_add(cur);
        schedule();
    }

    irq_restore(flags);
}

void sched_enqueue(thread_ctrl_blk_s* thread) {
    u32 flags = irq_save();
    thread->state = THREAD_STATE_READY;
    run_queue_add(thread);
    irq_restore(flags);
}

void sched_block(list_node_s* wait_queue, thread_state_e state) {
    u32 flags = irq_save();
    thread_ctrl_blk_s* cur = thread_self();
    cur->state             = state;
    list_add_to_tail(&cur->wait_link, wait_queue);
    schedule();
    irq_restore(flags);
}

void sched_block_and_release(list_node_s* wait_queue, thread_state_e state, spinlock_s* lock, u32 flags) {
    thread_ctrl_blk_s* cur = thread_self();
    cur->state             = state;
    list_add_to_tail(&cur->wait_link, wait_queue);
    spin_unlock(lock);
    schedule();
    irq_restore(flags);
}

void sched_unblock(thread_ctrl_blk_s* thread) {
    u32 flags = irq_save();
    list_del(&thread->wait_link);
    sched_ready(thread);
    irq_restore(flags);
}

void sched_yield(void) {
    u32 flags = irq_save();
    thread_ctrl_blk_s* cur = thread_self();
    cur->state             = THREAD_STATE_READY;
    cur->quantum           = SCHED_QUANTUM;
    run_queue_add(cur);
    schedule();
    irq_restore(flags);
}

void __noreturn sched_zombify(void) {
    irq_save();
    thread_ctrl_blk_s* cur = thread_self();
    cur->state             = THREAD_STATE_ZOMBIE;
    list_add_to_tail(&cur->run_link, &zombies);
    schedule();
    __builtin_unreachable();
}

void sched_reap_zombies(void) {
    u32 flags = irq_save();
    while (!list_is_empty(&zombies)) {
        thread_ctrl_blk_s* zombie = list_first_entry(
            &zombies, thread_ctrl_blk_s, run_link
        );

        list_del(&zombie->run_link);
        thread_destroy(zombie);
    }
    irq_restore(flags);
}

void __init sched_init(void) {
    for (u32 priority = 0; priority < SCHED_PRIORITY_CNT; priority++)
        list_init(&run_queue[priority]);

    bitmap = 0;
    list_init(&zombies);

    /* Register PIT timer IRQ handler */
    idt_register_handler(PIC_TO_INT(0), sched_pit_handler);
}

kernel_initcall(sched_init);