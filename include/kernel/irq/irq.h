#pragma once

#include <compiler.h>
#include <types.h>

typedef struct notification_s notification_s;

/**
 * irq_bind_notification - Binds @nt to interrupt @irq so that @nt is signalled,
 *                         with bit @irq set, each time the interrupt fires.
 * @irq: The interrupt number to bind.
 * @nt: The notification to signal.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_bind_notification(u8 irq, notification_s* nt);

/**
 * irq_init - Initializes the interrupt request (IRQ) subsystem.
 */
void __init irq_init(void);
