#pragma once

#include "types.h"

/**
 * irq_register_handler - Registers a handler for irq @irq.
 * @irq: The PIC IRQ number to register.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_register_handler(u8 irq);

/**
 * irq_wait - Blocks the running thread until @irq fires.
 * @irq: The PIC IRQ number to wait on.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_wait(u8 irq);

/**
 * irq_init - Initializes the interrupt request (IRQ) subsystem.
 */
void __init irq_init(void);