#pragma once

#include <stdint.h>

#include "../io/io.h"

/**
 * vga_enable_cursor - Enables the hardware text cursor.
 * @start: The starting scanline of the cursor.
 * @end: The ending scanline of the cursor.
 */
void __cold vga_enable_cursor(u8 start, u8 end);

/**
 * vga_disable_cursor - Disables the hardware text cursor.
 */
void __cold vga_disable_cursor(void);

/**
 * vga_print_hex - Prints a hexadecimal number at the current cursor position.
 * @num: The number to print.
 * @fcolor: The foreground color of the number.
 * @bcolor: The background color of the number.
 */
void __hot vga_print_hex(u32 num, u8 fcolor, u8 bcolor);

/**
 * vga_print_decimal - Prints a decimal number at the current cursor position.
 * @num: The number to print.
 * @fcolor: The foreground color of the number.
 * @bcolor: The background color of the number.
 */
void __hot vga_print_decimal(u32 num, u8 fcolor, u8 bcolor);

/**
 * vga_print_char - Prints a character at the current cursor position.
 * @c: The character to print.
 * @fcolor: The foreground color of the character.
 * @bcolor: The background color of the character.
 */
void __hot vga_print_char(char c, u8 fcolor, u8 bcolor);

/**
 * vga_print_string - Prints a string at the current cursor position.
 * @str: The string to print.
 * @fcolor: The foreground color of the string.
 * @bcolor: The background color of the string.
 */
void __hot vga_print_string(const char* str, u8 fcolor, u8 bcolor);

/**
 * vga_clear_screen - Clears the screen with a single color.
 * @color: The color to clear the screen with.
 */
void __cold vga_clear_screen(u8 color);