#pragma once

#include <stddef.h>

#include "types.h"

/* One-hot encoding for console selection */
#define KLOG_FLAG (1u << 0)               /* Selects kernel log console */
#define EVGA_FLAG (1u << 1)               /* Selects early VGA console  */
#define ALL_FLAG  (KLOG_FLAG | EVGA_FLAG) /* Selects all consoles       */

typedef struct console_t {
    u32  flags;                                 /* Selects console     */
    void (*putc)(const char c);                 /* Writes a character  */
    void (*puts)(const char* str);              /* Writes a string     */
    void (*write)(const char* str, size_t len); /* Writes a buffer     */
    void (*clear)(void);                        /* Clears console      */
    void (*init)(void);                         /* Initializes console */
} console_t;

typedef enum console_op_t {
    CONSOLE_OP_PUTC,  /* Writes a character  */
    CONSOLE_OP_PUTS,  /* Writes a string     */
    CONSOLE_OP_WRITE, /* Writes a buffer     */
    CONSOLE_OP_CLEAR, /* Clears console      */
    CONSOLE_OP_INIT   /* Initializes console */
} console_op_t;

/**
 * console_register_dev - Registers @dev.
 * @dev: The console to register.
 */
void console_register_dev(console_t dev);

/**
* console_unregister_dev - Unregisters consoles matching @flags.
* @flags: The flags selecting consoles to unregister.
*/
void console_unregister_dev(u32 flags);

/**
* console_exec - Executes @op on consoles matching @flags.
* @flags: The flags selecting consoles.
* @op: The operation to execute.
* @arg: The operation argument.
* @len: The buffer length.
*/
void console_exec(u32 flags, console_op_t op, const void* arg, size_t len);

/**
 * console_putc - Writes @c to consoles matching @flags.
 * @flags: The flags selecting consoles to write to.
 * @c: The character to write.
 */
void console_putc(u32 flags, const char c);

/**
 * console_puts - Writes @str to consoles matching @flags.
 * @flags: The flags selecting consoles to write to.
 * @str: The string to write.
 */
void console_puts(u32 flags, const char* str);

/**
 * console_write - Writes @buf to consoles matching @flags, up to @len bytes.
 * @flags: The flags selecting consoles to write to.
 * @buf: The buffer to write.
 * @len: The buffer length.
 */
void console_write(u32 flags, const char* buf, size_t len);

/**
 * console_clear - Clears consoles matching @flags.
 * @flags: The flags selecting consoles to clear.
 */
void console_clear(u32 flags);

/**
 * console_init - Initializes registered consoles.
 */
void __init console_init(void);