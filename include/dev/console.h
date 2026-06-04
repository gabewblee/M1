#pragma once

#include <stddef.h>

#include "types.h"

/* One-hot encoding for console selection */
#define KLOG_FLAG (1u << 0)               /* Selects kernel log console */
#define EVGA_FLAG (1u << 1)               /* Selects early VGA console  */
#define ALL_FLAG  (KLOG_FLAG | EVGA_FLAG) /* Selects all consoles       */

typedef struct console_s {
    u32  flags;                                 /* Selects console     */
    void (*putc)(const char c);                 /* Writes a character  */
    void (*puts)(const char* str);              /* Writes a string     */
    void (*write)(const char* str, size_t len); /* Writes a buffer     */
    void (*clear)(void);                        /* Clears console      */
    void (*init)(void);                         /* Initializes console */
} console_s;

/**
 * console_register_dev - Registers @dev.
 * @dev: The console to register.
 */
void console_register_dev(const console_s dev);

/**
 * console_unregister_dev - Unregisters the consoles matching @flags.
 * @flags: The flags selecting the consoles to unregister.
 */
void console_unregister_dev(u32 flags);

/**
 * console_putc - Writes @c to the consoles matching @flags.
 * @flags: The flags selecting the consoles to write to.
 * @c: The character to write.
 */
void console_putc(u32 flags, const char c);

/**
 * console_puts - Writes @str to the consoles matching @flags.
 * @flags: The flags selecting the consoles to write to.
 * @str: The string to write.
 */
void console_puts(u32 flags, const char* str);

/**
 * console_write - Writes @buf to the consoles matching @flags, up to @len bytes.
 * @flags: The flags selecting the consoles to write to.
 * @buf: The buffer to write.
 * @len: The buffer length.
 */
void console_write(u32 flags, const char* buf, size_t len);

/**
 * console_clear - Clears the consoles matching @flags.
 * @flags: The flags selecting the consoles to clear.
 */
void console_clear(u32 flags);

/**
 * console_init - Initializes the registered consoles.
 */
void __init console_init(void);