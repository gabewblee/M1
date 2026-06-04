#pragma once

#include "boot/multiboot.h"

/**
 * server_lookup - Looks up the task ID by @name.
 * @name: The NULL-terminated server name.
 * Returns: The task ID on success, or a negative error code on failure.
 */
i32 server_lookup(const char* name);

/**
 * servers_init - Initializes the userspace servers.
 * @mbinfo: The multiboot information structure.
 */
void servers_init(const multiboot_info_t* mbinfo);