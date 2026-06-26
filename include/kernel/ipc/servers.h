#pragma once

#include <boot/multiboot.h>
#include <types.h>

/**
 * server_lookup - Looks up the task ID associated with @id.
 * @id: The server ID to lookup.
 * Returns: The task ID on success, or a negative error code on failure.
 */
i32 server_lookup(u32 id);

/**
 * servers_init - Initializes the userspace servers.
 * @mbinfo: The multiboot information structure.
 */
void servers_init(multiboot_info_t* mbinfo);
