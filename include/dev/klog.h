#pragma once

#include <stddef.h>

/**
 * klog_putc - Writes a character to the kernel log.
 * @c: The character to write.
 */
void klog_putc(const char c);

/**
 * klog_puts - Writes a string to the kernel log.
 * @str: The string to write.
 */
void klog_puts(const char* str);

/**
 * klog_write - Writes a buffer to the kernel log.
 * @str: The buffer to write.
 * @len: The length of the buffer.
 */
void klog_write(const char* str, const size_t len);

/**
 * klog_clear - Clears the kernel log.
 */
void klog_clear(void);

/**
 * klog_init - Initializes the kernel log.
 */
void klog_init(void);
