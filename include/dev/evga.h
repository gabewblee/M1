#pragma once

#include <stddef.h>

#include "compiler.h"

/**
 * evga_putc - Writes a character to the boot VGA console.
 * @c: The character to write.
 */
void __init evga_putc(const char c);

/**
 * evga_puts - Writes a string to the boot VGA console.
 * @str: The string to write.
 */
void __init evga_puts(const char* str);

/**
 * evga_write - Writes a string to the boot VGA console.
 * @str: The string to write.
 * @len: The length of the string.
 */
void __init evga_write(const char* str, const size_t len);

/**
 * evga_clear - Clears the boot VGA console.
 */
void __init evga_clear(void);

/**
 * evga_init - Initializes the boot VGA console.
 */
void __init evga_init(void);
