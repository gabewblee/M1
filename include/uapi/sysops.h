#pragma once

#include <stddef.h>

#include "types.h"
#include "ipc.h"

#define SYSOP_log_read      0 /* System operation 0 */
#define SYSOP_server_lookup 1 /* System operation 1 */

typedef struct sysop_msg_t {
    u32 arg0; /* System operation argument 0 */
    u32 arg1; /* System operation argument 1 */
    u32 arg2; /* System operation argument 2 */
    u32 arg3; /* System operation argument 3 */
} sysop_msg_t;

_Static_assert(offsetof(sysop_msg_t, arg0) == 0, "Error: Invalid sysop argument 0 offset");
_Static_assert(offsetof(sysop_msg_t, arg1) == sizeof(u32), "Error: Invalid sysop argument 1 offset");
_Static_assert(offsetof(sysop_msg_t, arg2) == sizeof(u32) * 2u, "Error: Invalid sysop argument 2 offset");
_Static_assert(offsetof(sysop_msg_t, arg3) == sizeof(u32) * 3u, "Error: Invalid sysop argument 3 offset");
_Static_assert(sizeof(sysop_msg_t) == sizeof(u32) * 4u, "Error: Invalid sysop message size");