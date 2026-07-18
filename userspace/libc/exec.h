#pragma once

#include <uapi/types.h>

/**
 * root_spawn - Asks the root task to load and run the program named by
 *              @cmdline, blocking until the program exits.
 * @cmdline: The command line: an absolute program path, optionally followed
 *           by space-separated arguments.
 * Returns: E_OK once the program has exited, or a negative error code when
 *          it could not be started.
 */
i32 root_spawn(const char* cmdline);
