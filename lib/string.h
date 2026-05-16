#pragma once

#include <stddef.h>

#include "../config.h"

/**
 * memset - Fills a memory area with a constant byte.
 * @s: The pointer to the memory area.
 * @c: The value to fill the memory with.
 * @n: The number of bytes to fill.
 * @return: The pointer to the memory area.
 */
void* memset(void* s, int c, size_t n);