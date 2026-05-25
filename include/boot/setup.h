#pragma once

#include "config.h"

/**
 * setup_swapper_pg_dir - Initializes the swapper page directory. Sets up
 *                        identity mapping and higher-half mapping. Initial 
 *                        page tables are physically placed right after the
 *                        kernel image as the number of page tables required
 *                        must be dynamically determined.
 *
 * Context: Calculates the exact number of page tables required to map the
 *          entire kernel image. The function is placed in the .multiboot
 *          section because higher-half kernel is not mapped yet on function
 *          entry.
 */
void __multiboot setup_swapper_pg_dir(void);
