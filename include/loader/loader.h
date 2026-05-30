#pragma once

#include <stddef.h>

#include "types.h"

/**
 * load_elf_from_memory - Loads a static ELF32 i386 image into the
 *                        current address space.
 * @data: The pointer to the ELF image.
 * @sz: The image size.
 * Returns: The entry point on success, or 0 on failure.
 */
virt_addr_t load_elf_from_memory(const void* data, size_t sz);