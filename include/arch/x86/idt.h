#pragma once

#include "config.h"

#define IRQ_TO_VEC(irq) ((irq) + IDT_EXC_CNT) /* Converts an IRQ number to an interrupt vector */

typedef struct int_frm_s {
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; /* General-purpose registers               */
    u32 vec;                                    /* Interrupt vector                        */
    u32 err;                                    /* Error code                              */
    u32 eip, cs, eflags;                        /* Return address, code segment, and flags */
} int_frm_s;

typedef void (*int_handler_func_t)(const int_frm_s*);

/**
 * idt_register_handler - Registers handler @handler for interrupt
 *                        vector @vec.
 * @vec: The interrupt vector.
 * @handler: The handler to register.
 */
void idt_register_handler(u8 vec, int_handler_func_t handler);

/**
 * idt_unregister_handler - Unregisters handler for interrupt
 *                          vector @vec.
 * @vec: The interrupt vector.
 */
void idt_unregister_handler(u8 vec);

/**
 * int_handler - Dispatches @frm to its registered handler. Sends an 
 *               EOI signal if the interrupt is a PIC IRQ.
 * @frm: The pointer to the interrupt stack frame.
 */
void int_handler(const int_frm_s* frm);

/**
 * idt_init - Initializes the IDT. Registers the exception handlers,
 *            the IRQ handlers, and the syscall handler. Loads the IDT
 *            register.
 */
void __init idt_init(void);
