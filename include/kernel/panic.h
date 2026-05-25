#pragma once

#include "config.h"

#define PANIC(msg) panic(msg, __FILE__, __LINE__)

/**
 * panic - Prints a panic message, disables interrupts, and halts forever.
 * @msg: The panic message to print.
 * @file: The file where the panic occurred.
 * @line: The line where the panic occurred.
 *
 * Context: This function represents a unrecoverable kernel fault.
 */
void __noreturn panic(const char *msg, const char *file, u32 line);