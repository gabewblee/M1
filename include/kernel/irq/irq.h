#pragma once

#include <compiler.h>
#include <types.h>

typedef struct ntfn_s ntfn_s;

/**
 * irq_bind_ntfn - Binds @nt to interrupt @irq. Each time interrupt 
 *                 @irq fires, @nt is signalled with bit @irq set.
 * @irq: The interrupt number to bind.
 * @nt: The notification to signal.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 irq_bind_ntfn(u8 irq, ntfn_s* nt);
