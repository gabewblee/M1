#pragma once

#include <stddef.h>

#include "types.h"

#define KLOG_FLAG (1u << 0)               /* Selects the kernel log console */
#define EVGA_FLAG (1u << 1)              /* Selects the VGA console        */
#define ALL_FLAG  (KLOG_FLAG | EVGA_FLAG) /* Selects every console          */

typedef struct console_t {
    u32  flags;                                 /* Selects consoles        */
    void (*putc)(char c);                       /* Writes one character    */
    void (*puts)(const char* str);              /* Writes a string         */
    void (*write)(const char* str, size_t len); /* Writes a buffer         */
    void (*clear)(void);                        /* Clears the console      */
    void (*init)(void);                         /* Initializes the console */
} console_t;

typedef enum console_op_t {
    CONSOLE_OP_PUTC,  /* Writes a character  */
    CONSOLE_OP_PUTS,  /* Writes a string     */
    CONSOLE_OP_WRITE, /* Writes a buffer     */
    CONSOLE_OP_CLEAR, /* Clears console      */
    CONSOLE_OP_INIT   /* Initializes console */
} console_op_t;

/**
 * console_putc - Writes a character to the specified consoles.
 * @flags: The flags specifying the consoles to write to.
 * @c: The character to write.
 */
void console_putc(u32 flags, char c);

/**
 * console_puts - Writes a string to the specified consoles.
 * @flags: The flags specifying the consoles to write to.
 * @str: The string to write.
 */
void console_puts(u32 flags, const char* str);

/**
 * console_write - Writes a buffer to the specified consoles.
 * @flags: The flags specifying the consoles to write to.
 * @str: The buffer to write.
 * @len: The length of the buffer.
 */
void console_write(u32 flags, const char* str, size_t len);

/**
 * console_clear - Clears the specified consoles.
 * @flags: The flags specifying the consoles to clear.
 */
void console_clear(u32 flags);

/**
 * console_init - Initializes the registered consoles.
 */
void console_init(void);

/**
 * console_register_dev - Registers a console device.
 * @con: The console device to register.
 */
void console_register_dev(console_t* con);

/**
 * console_unregister_dev - Unregisters console devices matching @flags.
 * @flags: The flags selecting devices to unregister.
 */
void console_unregister_dev(u32 flags);

/**
 * console_exec - Executes a console operation on matching consoles.
 * @flags: The flags selecting consoles.
 * @op: The operation code to execute.
 * @arg: Operation argument (char for putc, const char* for puts/write).
 * @len: Buffer length for write.
 */
void console_exec(u32 flags, console_op_t op, const void* arg, size_t len);