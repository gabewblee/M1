#pragma once

#include "types.h"
#include "uapi/syscall.h"

typedef struct syscall_frm_s {
    u32 ebp;    /* Syscall frame base pointer                    */
    u32 ebx;    /* Syscall argument 0                            */
    u32 ecx;    /* Syscall argument 1                            */
    u32 edx;    /* Syscall argument 2                            */
    u32 esi;    /* Syscall argument 3                            */
    u32 edi;    /* Syscall argument 4                            */
    u32 eax;    /* Syscall number on entry, return value on exit */
    u32 eip;    /* Return address                                */
    u32 cs;     /* Code segment                                  */
    u32 eflags; /* EFLAGS register                               */
    u32 uesp;   /* User stack pointer                            */
    u32 uss;    /* User stack segment                            */
} syscall_frm_s;

/**
 * syscall_handler - Handles system calls.
 * @frm: The pointer to the syscall stack frame.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 syscall_handler(syscall_frm_s* frm);
