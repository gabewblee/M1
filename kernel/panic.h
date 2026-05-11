#pragma once

#include "../config.h"

/**
 * panic - Halt system and print panic message
 * @msg: The panic message to print
 * @file: The file where the panic occurred
 * @line: The line where the panic occurred
 */
void __noreturn panic(const char *msg, const char *file, u32 line);

#define PANIC(msg) panic(msg, __FILE__, __LINE__) /* Halt system and print panic message */