#pragma once

#include <uapi/types.h>

#define PATH_MAX 56u /* Absolute path bytes, incl. NUL, matches VFS_PATH_MAX */

/**
 * path_resolve - Resolves @input against @cwd into a normalized absolute path.
 * @cwd: The current working directory, absolute and normalized.
 * @input: The path to resolve, absolute or relative.
 * @output: The destination buffer.
 * @size: The buffer capacity in bytes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 path_resolve(const char* cwd, const char* input, char* output, u32 size);
