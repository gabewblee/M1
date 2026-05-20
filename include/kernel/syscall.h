#pragma once

#include "config.h"

typedef struct syscall_frm_t {
    u32 ebp;    /* Frame base pointer                            */
    u32 ebx;    /* Argument 5                                    */
    u32 ecx;    /* Argument 4                                    */
    u32 edx;    /* Argument 3                                    */
    u32 esi;    /* Argument 2                                    */
    u32 edi;    /* Argument 1                                    */
    u32 eax;    /* Syscall number on entry, return value on exit */
    u32 eip;    /* Return address                                */
    u32 cs;     /* Code segment                                  */
    u32 eflags; /* Flags                                         */
    u32 uesp;   /* User stack pointer                            */
    u32 uss;    /* User stack segment                            */
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
 * syscall_handler - Handles a system call.
 * @frm: The pointer to the system call stack frame.
 */
void syscall_handler(syscall_frm_t* frm);