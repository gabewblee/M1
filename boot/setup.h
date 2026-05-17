#pragma once

#include "../config.h"

/**
 * setup_swapper_pg_dir - Initializes swapper page directory. Sets up
 *                        identity mapping and higher-half mapping. Initial 
 *                        page tables are physically placed right after the
 *                        kernel image.
 */
void __multiboot setup_swapper_pg_dir(void);
