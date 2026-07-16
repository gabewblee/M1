#pragma once

#include <config.h>

#define PIC_TO_INT(irq) ((irq) + IDT_EXC_CNT) /* Converts a PIC interrupt number to an interrupt number */

typedef struct ifrm_s {
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; /* General purpose registers               */
    u32 vec;                                    /* Interrupt number                        */
    u32 err;                                    /* Error code                              */
    u32 eip, cs, eflags;                        /* Return address, code segment, and flags */
} ifrm_s;

typedef void (*ihandler_f)(ifrm_s*);

/**
 * idt_register_handler - Registers @ihandler for interrupt @vec.
 * @vec: The interrupt number to register.
 * @ihandler: The handler to register.
 */
void idt_register_handler(u8 vec, ihandler_f ihandler);

/**
 * idt_unregister_handler - Unregisters handler for interrupt @vec.
 * @vec: The interrupt number to unregister.
 */
void idt_unregister_handler(u8 vec);

/**
 * ihandler - Dispatches @frm to its registered handler. Sends an 
 *            EOI signal if the interrupt is a PIC interrupt.
 * @frm: The pointer to the interrupt stack frame.
 */
void ihandler(ifrm_s* frm);

