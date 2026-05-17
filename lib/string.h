#pragma once

#include <stddef.h>

#include "../config.h"

/**
 * memset - Fills a memory area with a constant byte.
 * @str: The pointer to the memory area.
 * @c: The value to fill the memory with.
 * @n: The number of bytes to fill.
 * @return: The pointer to the memory area.
 */
void* memset(void* str, int c, size_t n);

/**
 * memcpy - Copies @n bytes from @src to @dst. The regions must not overlap.
 * @dst: The destination buffer.
 * @src: The source buffer.
 * @n: The number of bytes to copy.
 * @return: The destination buffer.
 */
void* memcpy(void* dst, const void* src, size_t n);