#pragma once

#include <stddef.h>

/**
 * vga_putc - Writes a character to the screen.
 * @c: The character to write.
 */
void vga_putc(const char c);

/**
 * vga_puts - Writes a string to the screen.
 * @str: The string to write.
 */
void vga_puts(const char* str);

/**
 * vga_write - Writes a string to the screen.
 * @str: The string to write.
 * @len: The length of the string.
 */
void vga_write(const char* str, const size_t len);

/**
 * vga_clear - Clears the screen.
 */
void vga_clear(void);

/**
 * vga_init - Initializes the VGA device.
 */
void vga_init(void);
