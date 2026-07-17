#pragma once

#include <uapi/types.h>

#define LINE_MAX 128u /* Line buffer bytes, incl. NUL */

/**
 * line_read - Reads one line from the keyboard into @buffer, echoing input
 *             and handling backspace editing.
 * @buffer: The destination buffer.
 * @size: The buffer capacity in bytes.
 * Returns: The line length, or a negative error code on failure.
 */
i32 line_read(char* buffer, u32 size);
