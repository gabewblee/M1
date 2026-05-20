#pragma once

#include <stddef.h>

#include "types.h"

/**
 * load_elf_from_memory - Load a statically-linked ELF executable into memory.
 * @data: The pointer to theELF image.
 * @sz: The image size.
 * @return: The entry point on success, or 0 on failure.
 */
virt_addr_t load_elf_from_memory(const void* data, size_t sz);
