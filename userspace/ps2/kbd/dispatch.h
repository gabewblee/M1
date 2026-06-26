#pragma once

#include <uapi/uapi.h>
#include <userspace/server/server.h>

extern const server_handler_f kbd_handlers[]; /* PS/2 keyboard operation table */

/**
 * init - Initializes PS/2 keyboard server.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 init(void);

/**
 * fini - Finalizes PS/2 keyboard server. Currently a no-op.
 */
void fini(void);