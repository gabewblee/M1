#pragma once

#include <stddef.h>
#include <types.h>

/* One-hot encoding for console selection */
#define KLOG_FLAG   (1u << 0)                             /* Selects kernel log console  */
#define EVGA_FLAG   (1u << 1)                             /* Selects early VGA console   */
#define SERIAL_FLAG (1u << 2)                             /* Selects COM1 serial console */
#define ALL_FLAG    (KLOG_FLAG | EVGA_FLAG | SERIAL_FLAG) /* Selects all consoles        */

typedef struct console_s {
    u32  flags;                           /* Selects console     */
    void (*putc)(char c);                 /* Writes a character  */
    void (*puts)(char* str);              /* Writes a string     */
    void (*write)(char* buf, size_t len); /* Writes a buffer     */
    void (*clear)(void);                  /* Clears console      */
    void (*init)(void);                   /* Initializes console */
} console_s;

/**
 * console_register - Registers @con.
 * @con: The console to register.
 */
void console_register(console_s con);

/**
 * console_unregister - Unregisters consoles matching @flags.
 * @flags: KLOG_FLAG, EVGA_FLAG, or ALL_FLAG.
 */
void console_unregister(u32 flags);

/**
 * console_putc - Writes @c to the consoles matching @flags.
 * @flags: KLOG_FLAG, EVGA_FLAG, or ALL_FLAG.
 * @c: The character to write.
 */
void console_putc(u32 flags, char c);

/**
 * console_puts - Writes @str to the consoles matching @flags.
 * @flags: KLOG_FLAG, EVGA_FLAG, or ALL_FLAG.
 * @str: The string to write.
 */
void console_puts(u32 flags, char* str);

/**
 * console_write - Writes @buf to the consoles matching @flags, up to @len bytes.
 * @flags: KLOG_FLAG, EVGA_FLAG, or ALL_FLAG.
 * @buf: The buffer to write.
 * @len: The buffer length.
 */
void console_write(u32 flags, char* buf, size_t len);

/**
 * console_clear - Clears the consoles matching @flags.
 * @flags: KLOG_FLAG, EVGA_FLAG, or ALL_FLAG.
 */
void console_clear(u32 flags);

/**
 * console_init - Initializes the consoles.
 */
void __init console_init(void);
