#pragma once

#include <compiler.h>
#include <stddef.h>

/**
 * evga_putc - Writes @c to the early VGA console.
 * @c: The character to write.
 */
void __init evga_putc(char c);

/**
 * evga_puts - Writes @str to the early VGA console.
 * @str: The string to write.
 */
void __init evga_puts(char* str);

/**
 * evga_write - Writes @buf to the early VGA console, up to @len bytes.
 * @buf: The buffer to write.
 * @len: The buffer length.
 */
void __init evga_write(char* buf, size_t len);

/**
 * evga_clear - Clears the early VGA console.
 */
void __init evga_clear(void);

/**
 * evga_init - Initializes the early VGA console.
 */
void __init evga_init(void);
