#pragma once

/**
 * vfs_log - Writes formatted output (%s %c %u %d %x %%) to the kernel debug console.
 * @fmt: The format string.
 */
void vfs_log(const char* fmt, ...);
