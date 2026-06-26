#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <uapi/types.h>

/**
 * putc - Writes @c to the VGA console.
 * @c: The character to write.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 putc(int c);

/**
 * puts - Writes @str to the VGA console.
 * @str: The string to write.
 * Returns: The number of characters written on success, or a negative error
 *          code on failure.
 */
i32 puts(char* str);

/**
 * write - Writes @buf to the VGA console, up to @len bytes.
 * @buf: The buffer to write.
 * @len: The buffer length.
 * Returns: The number of bytes written on success, or a negative error code on
 *          failure.
 */
i32 write(char* buf, size_t len);

/**
 * clear - Clears the VGA console.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 clear(void);

/**
 * printf - Writes formatted output to the VGA console.
 * @fmt: The format string.
 * Returns: The number of characters written on success, or a negative error
 *          code on failure.
 */
i32 printf(char* fmt, ...);
