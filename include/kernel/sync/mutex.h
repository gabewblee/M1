#pragma once

#include <kernel/core/sched.h>
#include <kernel/core/thread.h>
#include <kernel/sync/spinlock.h>
#include <libk/list.h>

typedef struct mutex_s {
    spinlock_s  guard;   /* Protects locked and waiters */
    u32         locked;  /* 1 if held, 0 if free        */
    list_node_s waiters; /* Queue of blocked threads    */
} mutex_s;

/**
 * mutex_lock - Acquires @mtx, blocking until it is available.
 * @mtx: The mutex to acquire.
 */
static inline void mutex_lock(mutex_s* mtx) {
    u32 flags = spin_lock_irqsave(&mtx->guard);
    while (mtx->locked) {
        sched_block(&mtx->waiters, THREAD_STATE_BLOCKED);
        flags = spin_lock_irqsave(&mtx->guard);
    }
    mtx->locked = 1;
    spin_unlock_irqrestore(&mtx->guard, flags);
}

/**
 * mutex_unlock - Releases @mtx, waking the next waiter.
 * @mtx: The mutex to release.
 */
static inline void mutex_unlock(mutex_s* mtx) {
    u32 flags = spin_lock_irqsave(&mtx->guard);
    mtx->locked = 0;
    if (!list_is_empty(&mtx->waiters)) {
        thread_ctrl_blk_s* next = get_container_of(
            mtx->waiters.next, thread_ctrl_blk_s, wait_link
        );
        sched_unblock(next);
    }
    spin_unlock_irqrestore(&mtx->guard, flags);
}

/**
 * mutex_init - Initializes @mtx.
 * @mtx: The mutex to initialize.
 */
static inline void mutex_init(mutex_s* mtx) {
    spinlock_init(&mtx->guard);
    mtx->locked = 0;
    list_init(&mtx->waiters);
}

typedef struct mutex_guard_s {
    mutex_s* mtx;
} mutex_guard_s;

static inline mutex_guard_s mutex_guard_acquire(mutex_s* mtx) {
    mutex_lock(mtx);
    return (mutex_guard_s){
        .mtx = mtx
    };
}

static inline void mutex_guard_release(mutex_guard_s* g) {
    mutex_unlock(g->mtx);
}

/**
 * mutex_guard - Acquires @mtx, automatically releasing it at end of scope.
 * @mtx: The mutex to acquire.
 */
#define mutex_guard(mtx)                                \
    mutex_guard_s __mutex_guard_##__COUNTER__           \
        __attribute__((cleanup(mutex_guard_release))) = \
        mutex_guard_acquire(mtx)