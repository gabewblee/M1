#pragma once

#include "types.h"

/**
 * server_lookup - Retrieves the task ID associated with @name.
 * @name: The NULL-terminated server name.
 * Returns: The task ID on success, or a negative error code on failure.
 */
i32 server_lookup(const char* name);

/**
 * services_init - Initializes all userspace servers. For each server,
 *                 it allocates a user stack, creates a task, loads the
 *                 server's ELF image from memory, maps the user stack,
 *                 and creates a user thread to run the server.
 *
 * Supports:
 * - VGA server
 *
 * Context: A microkernel is, by definition, a kernel that only supports
 *          a minimal set of services, such as IPC, interrupts, and
 *          scheduling. All non-essential services such as drivers,
 *          filesystems, and other applications are implemented as userspace 
 *          servers. This function effectively bootstraps the userspace
 *          servers.
 */
void servers_init(void);
