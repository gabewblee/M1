#pragma once

#include <uapi/uapi.h>
#include <userspace/server/server.h>

extern const server_handler_f vfs_handlers[]; /* VFS operation table */

/**
 * init - Initializes the VFS server: heap, caches, root mount, self-test.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 init(void);

/**
 * fini - Finalizes VFS server. Currently a no-op.
 */
void fini(void);
