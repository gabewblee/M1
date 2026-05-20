#pragma once

#include "types.h"

/* Expands to the int 0x80 syscall table; X(id, name, argc). */
#define SYSCALLS(X)              \
    X(0, ipc_send,  2)           /* Send a message to a task port        */ \
    X(1, ipc_recv,  1)           /* Receive a message from the task port */ \
    X(2, ipc_call,  2)           /* Send a request and wait for a reply  */ \
    X(3, ipc_reply, 2)           /* Deliver a reply to a client thread   */ \
    X(4, max,       0)           /* Sentinel past the last syscall       */

/* Expands to kernel operations invoked through ipc_call; X(id, name, argc). */
#define SYSOPS(X)                      \
    X(0, thread_create,   2)           /* Create a thread in the caller task */ \
    X(1, thread_yield,    0)           /* Voluntarily yield the CPU          */ \
    X(2, thread_exit,     1)           /* Terminate the calling thread       */ \
    X(3, log_read,        3)           /* Read from the kernel log buffer    */ \
    X(4, map_pg,          3)           /* Map a page in the caller aspace    */ \
    X(5, unmap_pg,        1)           /* Unmap a page in the caller aspace  */

#define IPC_INLINE_SZ  56u              /* Maximum inline IPC payload size */
#define VM_FLAG_RW     0x002u           /* Page is writable                 */
#define VM_FLAG_USER   0x004u           /* Page is user-accessible          */
#define KERNEL_TASK_ID 0u               /* Task id reserved for the kernel  */

typedef enum syscall_no_t {
/* Expands to one syscall_no_t enumerator. */
#define SYSCALL_ENUM(id, name, argc) SYS_##name = (id),
    SYSCALLS(SYSCALL_ENUM)
#undef SYSCALL_ENUM
} syscall_no_t;

typedef enum syscall_err_t {
    E_OK    = 0, /* Operation succeeded                    */
    E_INVAL = 1, /* Invalid argument                       */
    E_NOMEM = 2, /* Out of memory                          */
    E_NOSYS = 3, /* Unsupported operation                  */
    E_AGAIN = 4, /* Resource temporarily unavailable       */
    E_FAULT = 5, /* User buffer not accessible             */
    E_PERM  = 6  /* Permission denied                      */
} syscall_err_t;

typedef enum sysop_no_t {
/* Expands to one sysop_no_t enumerator. */
#define SYSOP_ENUM(id, name, argc) SYSOP_##name = (id),
    SYSOPS(SYSOP_ENUM)
#undef SYSOP_ENUM
} sysop_no_t;

typedef struct sysop_msg_t {
    u32 arg0; /* Argument 0 */
    u32 arg1; /* Argument 1 */
    u32 arg2; /* Argument 2 */
    u32 arg3; /* Argument 3 */
} sysop_msg_t;

_Static_assert(sizeof(sysop_msg_t) <= IPC_INLINE_SZ, "Error: sysop_msg_t exceeds IPC inline payload size");

typedef struct ipc_msg_t {
    u32 id;                  /* Message identifier                           */
    u32 sender;              /* Originating thread id, kernel-filled on recv */
    u32 sz;                  /* Valid payload size in bytes                  */
    u8  data[IPC_INLINE_SZ]; /* Inline payload                               */
} ipc_msg_t;

_Static_assert(sizeof(ipc_msg_t) == 68u, "Error: ipc_msg_t ABI drift");
