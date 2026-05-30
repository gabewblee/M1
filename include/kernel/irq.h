#pragma once

#include "types.h"

/**
 * irq_register_handler - Registers a dispatch function for @irq. Each time
 *                        @irq fires, the dispatch function is called, which
 *                        wakes up all threads waiting on @irq.
 * @irq: The PIC IRQ number to register.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_register_handler(u8 irq);

/**
 * irq_wait - Blocks the running thread on @irq's wait queue.
 * @irq: The PIC IRQ number to wait on.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_wait(u8 irq);

/**
 * irq_init - Initializes IRQ wait queues.
 *
 * Context: The main idea is that the wait queues hold blocked
 *          threads waiting for an IRQ to fire. When an IRQ fires,
 *          its blocked threads are woken up and re-scheduled. The
 *          reason for doing this is to avoid wasting CPU time when
 *          polling for an IRQ.
 */
void __init irq_init(void);