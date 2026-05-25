#pragma once

#include <stddef.h>

#include "types.h"

#define VGA_COLS 80      /* VGA column count            */
#define VGA_ROWS 25      /* VGA row count               */
#define VGA_ADDR 0xB8000 /* VGA physical memory address */

/**
 * vga_putc - Writes @c to the VGA console.
 * @c: The character to write.
 */
void vga_putc(const char c);

/**
 * vga_puts - Writes @str to the VGA console.
 * @str: The string to write.
 */
void vga_puts(const char* str);

/**
 * vga_write - Writes @str to the VGA console, up to @len bytes.
 * @str: The buffer to write.
 * @len: The buffer length.
 */
void vga_write(const char* str, size_t len);

/**
 * vga_clear - Clears the VGA console.
 */
void vga_clear(void);

/**
 * vga_init - Initializes the VGA console.
 */
void vga_init(void);