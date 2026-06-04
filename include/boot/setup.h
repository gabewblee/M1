#pragma once

#include "config.h"

/**
 * setup_swapper_pg_dir - Initializes the swapper page directory. Sets up 
 *                        identity mapping and higher-half mappings.
 *
 * Preconditions:
 * - ekernel is 4 KiB-aligned.
 */
void __multiboot setup_swapper_pg_dir(void);
