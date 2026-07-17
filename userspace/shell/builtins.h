#pragma once

#include <uapi/types.h>
#include <userspace/shell/path.h>

#define SHELL_ARG_MAX 8u /* Arguments per command, incl. name */

typedef struct shell_state_s {
    char cwd[PATH_MAX]; /* Current working directory */
} shell_state_s;

typedef i32 (*builtin_f)(shell_state_s* shell, u32 argc, char** argv);

typedef struct builtin_s {
    const char* name;  /* Command name        */
    const char* usage; /* Argument synopsis   */
    const char* help;  /* One-line summary    */
    u32         min;   /* Minimum argc        */
    u32         max;   /* Maximum argc        */
    builtin_f   run;   /* Command entry point */
} builtin_s;

/**
 * builtin_find - Returns the builtin named @name, or NULL.
 * @name: The command name to look up.
 * Returns: The builtin, or NULL if unknown.
 */
const builtin_s* builtin_find(const char* name);

/**
 * builtin_list - Prints the name and summary of every builtin.
 */
void builtin_list(void);
