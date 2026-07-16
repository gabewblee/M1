#pragma once

#include <stddef.h>

/**
 * memset - Fills a memory area with a constant byte.
 * @s: The pointer to the memory area.
 * @c: The value to fill the memory with.
 * @n: The number of bytes to fill.
 * Returns: The pointer to the memory area.
 */
void* memset(void* s, int c, size_t n);

/**
 * memcpy - Copies @n bytes from @src to @dst.
 * @dst: The destination buffer.
 * @src: The source buffer.
 * @n: The number of bytes to copy.
 * Returns: The destination buffer.
 */
void* memcpy(void* dst, const void* src, size_t n);

/**
 * memcmp - Compares @n bytes of @a and @b.
 * @a: The first buffer.
 * @b: The second buffer.
 * @n: The number of bytes to compare.
 * Returns: 0 if the buffers are equal, a negative value if @a sorts before
 *          @b, or a positive value if @a sorts after @b.
 */
int memcmp(const void* a, const void* b, size_t n);

/**
 * strcmp - Compares two NUL-terminated strings.
 * @a: The first string.
 * @b: The second string.
 * Returns: 0 if the strings are equal, a negative value if @a sorts before
 *          @b, or a positive value if @a sorts after @b.
 */
int strcmp(const char* a, const char* b);

/**
 * strlen - Returns the length of @s.
 * @s: The string to measure.
 * Returns: The length of @s.
 */
size_t strlen(const char* s);
