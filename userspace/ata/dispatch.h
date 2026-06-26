#pragma once

#include <uapi/uapi.h>
#include <userspace/server/server.h>

extern const server_handler_f ata_handlers[]; /* ATA operation table */

/**
 * init - Initializes ATA server.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 init(void);

/**
 * fini - Finalizes ATA server. Currently a no-op.
 */
void fini(void);