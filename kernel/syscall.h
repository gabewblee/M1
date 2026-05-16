#pragma once

#include "../config.h"

typedef struct syscall_frm_t {
    u32 ebp;
    u32 ebx, ecx, edx, esi, edi;
    u32 eax;
    u32 eip, cs, eflags;
    u32 uesp;
    u32 uss;
} syscall_frm_t;

typedef enum syscall_err_t {
    E_OK    = 0,
    E_INVAL = 1,
    E_NOMEM = 2,
    E_NOSYS = 3,
    E_AGAIN = 4,
    E_FAULT = 5,
    E_PERM  = 6
} syscall_err_t;

/**
 * syscall_handler - System call handler.
 * @frm: The pointer to the system call stack frame.
 */
void __hot syscall_handler(syscall_frm_t* frm);