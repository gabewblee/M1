#pragma once

#include <config.h>

#define PIC_TO_INT(irq) ((irq) + IDT_EXC_CNT) /* Converts a PIC interrupt number to an interrupt number */

typedef struct int_frm_s {
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; /* General purpose registers               */
    u32 vec;                                    /* Interrupt number                        */
    u32 err;                                    /* Error code                              */
    u32 eip, cs, eflags;                        /* Return address, code segment, and flags */
} int_frm_s;

typedef void (*int_handler_f)(int_frm_s*);

/**
 * idt_register_handler - Registers @handler for interrupt @vec.
 * @vec: The interrupt number to register.
 * @handler: The handler to register.
 */
void idt_register_handler(u8 vec, int_handler_f handler);

/**
 * idt_unregister_handler - Unregisters handler for interrupt @vec.
 * @vec: The interrupt number to unregister.
 */
void idt_unregister_handler(u8 vec);

/**
 * int_handler - Dispatches @frm to its registered handler. Sends an 
 *               EOI signal if the interrupt is a PIC interrupt.
 * @frm: The pointer to the interrupt stack frame.
 */
void int_handler(int_frm_s* frm);

/**
 * idt_init - Initializes the interrupt descriptor table (IDT).
 */
void __init idt_init(void);
