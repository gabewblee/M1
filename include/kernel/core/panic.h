#pragma once

#include "config.h"

#define PANIC(msg) panic(msg, __FILE__, __LINE__)

/**
 * panic - Halts the kernel after an unrecoverable fault.
 * @msg: The panic message to print.
 * @file: The file where the panic occurred.
 * @line: The line where the panic occurred.
 *
 */
void __noreturn panic(const char *msg, const char *file, u32 line);