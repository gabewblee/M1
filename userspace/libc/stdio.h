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
 * puts - Writes @str and a trailing newline to the VGA console.
 * @str: The string to write.
 * Returns: The number of characters written on success, or a negative error
 *          code on failure.
 */
i32 puts(const char* str);

/**
 * write - Writes @buf to the VGA console, up to @len bytes.
 * @buf: The buffer to write.
 * @len: The buffer length.
 * Returns: The number of bytes written on success, or a negative error code on
 *          failure.
 */
i32 write(const char* buf, size_t len);

/**
 * clear - Clears the VGA console.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 clear(void);

/**
 * vsnprintf - Formats @fmt with @args into @buf, writing at most @size bytes.
 *             Supports %c, %s, %d, %u, %x, and %% with optional field width
 *             and zero padding. The output is always NUL-terminated when
 *             @size is nonzero.
 * @buf: The destination buffer.
 * @size: The destination buffer size in bytes.
 * @fmt: The format string.
 * @args: The format arguments.
 * Returns: The number of characters that would have been written, excluding
 *          the trailing NUL.
 */
i32 vsnprintf(char* buf, size_t size, const char* fmt, va_list args);

/**
 * snprintf - Formats into @buf, writing at most @size bytes.
 * @buf: The destination buffer.
 * @size: The destination buffer size in bytes.
 * @fmt: The format string.
 * Returns: The number of characters that would have been written, excluding
 *          the trailing NUL.
 */
i32 snprintf(char* buf, size_t size, const char* fmt, ...);

/**
 * printf - Writes formatted output to the VGA console.
 * @fmt: The format string.
 * Returns: The number of characters written on success, or a negative error
 *          code on failure.
 */
i32 printf(const char* fmt, ...);

/**
 * dprintf - Writes formatted output to the kernel debug console.
 * @fmt: The format string.
 * Returns: The number of characters written on success, or a negative error
 *          code on failure.
 */
i32 dprintf(const char* fmt, ...);
