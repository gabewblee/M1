#pragma once

#include <uapi/uapi.h>
#include <userspace/server/server.h>

extern const server_handler_f vga_handlers[]; /* VGA operation table */

/**
 * init - Initializes VGA server. Maps VGA memory.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 init(void);

/**
 * fini - Finalizes VGA server. Unmaps VGA memory.
 */
void fini(void);