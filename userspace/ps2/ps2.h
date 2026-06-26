#pragma once

#include <stdbool.h>
#include <uapi/types.h>

/**
 * ps2_data_ready - Checks if data is ready on the PS/2 data port.
 * Returns: true if data is ready, false otherwise.
 */
bool ps2_data_ready(void);

/**
 * ps2_read_data - Reads a byte from the PS/2 data port.
 * Returns: The byte read.
 */
u8 ps2_read_data(void);