#pragma once

#include <stdint.h>

#include "../config.h"

typedef struct interrupt_frm_t {
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 int_no;
    u32 err_code;
    u32 eip, cs, eflags;
} interrupt_frm_t;

/**
 * exception_handler - Exception handler.
 * @frm: The pointer to the interrupt stack frame.
 */
void exception_handler(interrupt_frm_t* frm);

/**
 * irq_handler - Interrupt handler.
 * @frm: The pointer to the interrupt stack frame.
 */
void irq_handler(interrupt_frm_t* frm);

/**
 * idt_init - Initialize the IDT.
 */
void __init idt_init(void);