#pragma once

#include "../config.h"

/**
 * gdt_init - Initializes the global descriptor table. Loads
 *            the task state segment descriptor to enable
 *            task switching.
 */
void __init gdt_init(void);