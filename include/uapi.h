#pragma once

#include <stddef.h>

#include "types.h"

/*--- Memory management --------------------------------------------------*/

#define PG_FLAG_RW     0x002u /* Page is writable        */
#define PG_FLAG_USER   0x004u /* Page is user-accessible */
#define PG_FLAG_GLOBAL 0x100u /* Page is global          */

/*--- IPC ----------------------------------------------------------------*/

#define IPC_MSG_SZ      (sizeof(u32) * 3u + IPC_PAYLOAD_SZ)
#define IPC_PAYLOAD_SZ  56u /* IPC payload size */

typedef struct ipc_msg_t {
    u32 id;                   /* Message ID          */
    u32 sender;               /* Sender's thread ID  */
    u32 sz;                   /* Payload size        */
    u8  data[IPC_PAYLOAD_SZ]; /* Inline payload data */
} ipc_msg_t;

_Static_assert(sizeof(ipc_msg_t) == IPC_MSG_SZ, "Error: ipc_msg_t size is not IPC_MSG_SZ");

/*--- Syscalls -----------------------------------------------------------*/

#define KERNEL_TASK_ID 0u /* Kernel task ID */

#define SYSCALLS(X)        \
    X(0, ipc_send,      2) \
    X(1, ipc_recv,      1) \
    X(2, ipc_call,      2) \
    X(3, ipc_reply,     2) \
    X(4, thread_create, 2) \
    X(5, thread_yield,  0) \
    X(6, thread_exit,   1) \
    X(7, map_pg,        3) \
    X(8, unmap_pg,      1) \
    X(9, max,           0)

typedef enum syscall_no_t {
#define SYSCALL_ENUM(id, name, argc) SYS_##name = (id),
    SYSCALLS(SYSCALL_ENUM)
#undef SYSCALL_ENUM
} syscall_no_t;

typedef enum syscall_err_t {
    E_OK    = 0, /* Success                 */
    E_INVAL = 1, /* Invalid argument        */
    E_NOMEM = 2, /* Allocation failure      */
    E_NOSYS = 3, /* Unsupported operation   */
    E_AGAIN = 4, /* Temporarily unavailable */
    E_FAULT = 5, /* Invalid address         */
    E_PERM  = 6  /* Invalid permissions     */
} syscall_err_t;

/*--- Sysops -------------------------------------------------------------*/

#define SYSOPS(X)          \
    X(0, log_read,      3) \
    X(1, server_lookup, 2)

typedef enum sysop_no_t {
#define SYSOP_ENUM(id, name, argc) SYSOP_##name = (id),
    SYSOPS(SYSOP_ENUM)
#undef SYSOP_ENUM
} sysop_no_t;

typedef struct sysop_msg_t {
    u32 arg0; /* System operation argument 0 */
    u32 arg1; /* System operation argument 1 */
    u32 arg2; /* System operation argument 2 */
    u32 arg3; /* System operation argument 3 */
} sysop_msg_t;

_Static_assert(sizeof(sysop_msg_t) <= IPC_PAYLOAD_SZ, "Error: sysop_msg_t size is greater than IPC_PAYLOAD_SZ");