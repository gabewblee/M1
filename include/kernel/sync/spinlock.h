#pragma once

#include <config.h>

typedef struct spinlock_s {
    _Atomic u32 locked;
} spinlock_s;

/**
 * spin_lock - Acquires @lk, spinning until it is available.
 * @lk: The spinlock to acquire.
 */
static inline void spin_lock(spinlock_s* lk) {
    while (__atomic_test_and_set(&lk->locked, __ATOMIC_ACQUIRE));
}

/**
 * spin_unlock - Releases @lk.
 * @lk: The spinlock to release.
 */
static inline void spin_unlock(spinlock_s* lk) {
    __atomic_clear(&lk->locked, __ATOMIC_RELEASE);
}

/**
 * spin_lock_irqsave - Disables local interrupts and acquires @lk.
 * @lk: The spinlock to acquire.
 * Returns: The caller's EFLAGS before interrupts were disabled.
 */
static inline u32 spin_lock_irqsave(spinlock_s* lk) {
    u32 flags;
    __asm__ volatile ("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    spin_lock(lk);
    return flags;
}

/**
 * spin_unlock_irqrestore - Releases @lk and restores EFLAGS.
 * @lk: The spinlock to release.
 * @flags: The EFLAGS value returned by spin_lock_irqsave().
 */
static inline void spin_unlock_irqrestore(spinlock_s* lk, u32 flags) {
    spin_unlock(lk);
    __asm__ volatile ("push %0; popf" : : "r"(flags) : "memory", "cc");
}

/**
 * spinlock_init - Initializes @lk.
 * @lk: The spinlock to initialize.
 */
static inline void spinlock_init(spinlock_s* lk) {
    __atomic_clear(&lk->locked, __ATOMIC_RELAXED);
}

typedef struct spinlock_guard_s {
    spinlock_s* lk;
    u32         flags;
} spinlock_guard_s;

static inline spinlock_guard_s spinlock_guard_acquire(spinlock_s* lk) {
    return (spinlock_guard_s){
        .lk = lk,
        .flags = spin_lock_irqsave(lk)
    };
}

static inline void spinlock_guard_release(spinlock_guard_s* g) {
    spin_unlock_irqrestore(g->lk, g->flags);
}

/**
 * spin_guard - Acquires @lk, automatically releasing it at end of scope.
 * @lk: The spinlock to acquire.
 */
#define spin_guard(lk)                                     \
    spinlock_guard_s __spin_guard_##__COUNTER__            \
        __attribute__((cleanup(spinlock_guard_release))) = \
        spinlock_guard_acquire(lk)
