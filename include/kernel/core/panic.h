#pragma once

#include <config.h>

#define PANIC(msg) panic(msg, __FILE__, __LINE__)

/**
 * panic - Prints panic messages, clears interrupts, and halts the kernel.
 * @msg: The panic message to print.
 * @file: The file where the panic occurred.
 * @line: The line where the panic occurred.
 */
void __noreturn panic(char *msg, char *file, u32 line);
