#pragma once

#include <stddef.h>

#include "ipc.h"
#include "types.h"

#define SERVER_MAX_NAME_LEN        16u /* Maximum server name length         */

#define SYSOP_log_read             0u  /* Sysop 0: Reads the kernel log      */
#define SYSOP_server_lookup        1u  /* Sysop 1: Looks up a server by name */
#define SYSOP_irq_register_handler 2u  /* Sysop 2: Registers an IRQ handler  */
#define SYSOP_irq_wait             3u  /* Sysop 3: Waits for an IRQ          */

typedef struct sysop_msg_s {
    u32 arg0; /* Argument 0 */
    u32 arg1; /* Argument 1 */
    u32 arg2; /* Argument 2 */
    u32 arg3; /* Argument 3 */
} sysop_msg_s;

_Static_assert(offsetof(sysop_msg_s, arg0) == 0, "Error: Invalid sysop argument 0 offset");
_Static_assert(offsetof(sysop_msg_s, arg1) == sizeof(u32), "Error: Invalid sysop argument 1 offset");
_Static_assert(offsetof(sysop_msg_s, arg2) == sizeof(u32) * 2u, "Error: Invalid sysop argument 2 offset");
_Static_assert(offsetof(sysop_msg_s, arg3) == sizeof(u32) * 3u, "Error: Invalid sysop argument 3 offset");
_Static_assert(sizeof(sysop_msg_s) == sizeof(u32) * 4u, "Error: Invalid sysop message size");