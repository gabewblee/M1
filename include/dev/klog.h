#pragma once

#include <stddef.h>

/**
 * klog_read - Reads at most @len bytes from the kernel log ring
 *             buffer, starting at @off.
 * @dst: The output buffer.
 * @len: The maximum number of bytes to read.
 * @off: The offset from the oldest available byte.
 * Returns: The number of bytes copied to @dst.
 */
size_t klog_read(char* dst, size_t len, size_t off);

/**
 * klog_putc - Writes @c to the kernel log.
 * @c: The character to write.
 */
void klog_putc(const char c);

/**
 * klog_puts - Writes @str to the kernel log.
 * @str: The string to write.
 */
void klog_puts(const char* str);

/**
 * klog_write - Writes @buf to the kernel log, up to @len bytes.
 * @buf: The buffer to write.
 * @len: The buffer length.
 */
void klog_write(const char* buf, const size_t len);

/**
 * klog_clear - Clears the kernel log.
 */
void klog_clear(void);

/**
 * klog_init - Initializes the kernel log.
 */
void __init klog_init(void);