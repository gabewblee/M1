#pragma once

#include <stdint.h>

#include "config.h"

typedef struct interrupt_frm_t {
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; /* General-purpose registers               */
    u32 int_no;                                 /* Interrupt number                        */
    u32 err_code;                               /* Error code                              */
    u32 eip, cs, eflags;                        /* Return address, code segment, and flags */
} interrupt_frm_t;

/**
 * exception_handler - Handles CPU exceptions.
 * @frm: The pointer to the interrupt stack frame.
 */
void exception_handler(interrupt_frm_t* frm);

/**
 * irq_handler - Handles IRQs.
 * @frm: The pointer to the interrupt stack frame.
 */
void irq_handler(interrupt_frm_t* frm);

/**
 * idt_init - Initializes the IDT.
 */
void __init idt_init(void);