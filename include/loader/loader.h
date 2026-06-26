#pragma once

#include <stddef.h>
#include <types.h>
#include <uapi/servers.h>

/**
 * load_server_desc - Loads the server descriptor note in an ELF32 image.
 * @data: The ELF image.
 * @sz: The image size.
 * Returns: The server descriptor on success, or NULL on failure.
 */
 const server_desc_s* load_server_desc(void* data, size_t sz);

/**
 * load_elf32 - Loads an ELF32 image into the current address space.
 * @data: The ELF image.
 * @sz: The image size.
 * Returns: The entry point on success, or 0 on failure.
 */
virt_addr_t load_elf32(void* data, size_t sz);