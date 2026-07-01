#pragma once

#include <stddef.h>

/**
 * serial_putc - Writes @c to the COM1 serial console.
 * @c: The character to write.
 */
void serial_putc(char c);

/**
 * serial_puts - Writes @str to the COM1 serial console.
 * @str: The string to write.
 */
void serial_puts(char* str);

/**
 * serial_write - Writes @buf to the COM1 serial console, up to @len bytes.
 * @buf: The buffer to write.
 * @len: The buffer length.
 */
void serial_write(char* buf, size_t len);

/**
 * serial_clear - Clears the COM1 serial console. No-op.
 */
void serial_clear(void);

/**
 * serial_init - Initializes the COM1 serial console.
 */
void serial_init(void);
