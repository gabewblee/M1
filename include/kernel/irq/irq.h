#pragma once

#include <compiler.h>
#include <types.h>

/**
 * irq_register_handler - Registers a handler for interrupt @irq. The handler
 *                        unblocks all threads waiting for interrupt @irq.
 * @irq: The interrupt number to register.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_register_handler(u8 irq);

/**
 * irq_wait_for - Waits for interrupt @irq to fire.
 * @irq: The interrupt number to wait for.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_wait_for(u8 irq);

/**
 * irq_init - Initializes the interrupt request (IRQ) subsystem.
 */
void __init irq_init(void);
