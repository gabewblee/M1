#pragma once

#include "config.h"

#define KERNEL_TASK_ID 0u /* Kernel task id */

typedef struct syscall_frm_t {
    u32 ebp;    /* Frame base pointer                            */
    u32 ebx;    /* Syscall argument 1                            */
    u32 ecx;    /* Syscall argument 2                            */
    u32 edx;    /* Syscall argument 3                            */
    u32 esi;    /* Syscall argument 4                            */
    u32 edi;    /* Syscall argument 5                            */
    u32 eax;    /* Syscall number on entry, return value on exit */
    u32 eip;    /* Return address                                */
    u32 cs;     /* Code segment                                  */
    u32 eflags; /* Flags                                         */
    u32 uesp;   /* User stack pointer                            */
    u32 uss;    /* User stack segment                            */
} syscall_frm_t;

typedef enum syscall_err_t {
    E_OK    = 0, /* Operation succeeded              */
    E_INVAL = 1, /* Invalid argument                 */
    E_NOMEM = 2, /* Out of memory                    */
    E_NOSYS = 3, /* Unsupported operation            */
    E_AGAIN = 4, /* Resource temporarily unavailable */
    E_FAULT = 5, /* User buffer not accessible       */
    E_PERM  = 6  /* Permission denied                */
} syscall_err_t;

typedef enum syscall_no_t {
    SYS_ipc_send  = 0, /* Sends a message to a task port        */
    SYS_ipc_recv  = 1, /* Receives a message from the task port */
    SYS_ipc_call  = 2, /* Sends a request and waits for a reply */
    SYS_ipc_reply = 3, /* Delivers a reply to a client thread   */
    SYS_max       = 4  /* Maximum number of system calls        */
} syscall_no_t;

typedef enum sysop_no_t {
    SYSOP_thread_create = 0, /* Creates a thread in the caller task         */
    SYSOP_thread_yield  = 1, /* Voluntarily yield the CPU                   */
    SYSOP_thread_exit   = 2, /* Terminate the calling thread                */
    SYSOP_log_read      = 3, /* Read from the kernel log buffer             */
    SYSOP_map_pg        = 4, /* Map a virtual address to a physical address */
    SYSOP_unmap_pg      = 5  /* Unmap a virtual address                     */
} sysop_no_t;

typedef struct sysop_msg_t {
    u32 arg0; /* Argument 0 */
    u32 arg1; /* Argument 1 */
    u32 arg2; /* Argument 2 */
    u32 arg3; /* Argument 3 */
} sysop_msg_t;

/**
 * syscall_handler - Dispatches int 0x80 system calls and records the frame on
 *                   the current thread.
 * @frm: The user-mode register frame pushed by the syscall stub.
 */
void syscall_handler(syscall_frm_t* frm);
