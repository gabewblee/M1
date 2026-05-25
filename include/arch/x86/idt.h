#pragma once

#include "config.h"

#define IRQ_TO_INT(irq) ((irq) + IDT_EXC_CNT)

typedef struct int_frm_t {
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; /* General-purpose registers               */
    u32 int_no;                                 /* Interrupt number                        */
    u32 err_code;                               /* Error code                              */
    u32 eip, cs, eflags;                        /* Return address, code segment, and flags */
} int_frm_t;

typedef void (*int_handler_func_t)(const int_frm_t*);

/**
 * idt_register_handler - Registers @handler for interrupt @vector.
 * @vector: The interrupt vector.
 * @handler: The handler to register.
 */
void idt_register_handler(u8 vector, int_handler_func_t handler);

/**
 * idt_unregister_handler - Unregisters handler for interrupt @vector.
 * @vector: The interrupt vector.
 */
void idt_unregister_handler(u8 vector);

/**
 * int_handler - Dispatches @frm to the registered handler.
 * @frm: The pointer to the interrupt stack frame.
 */
void int_handler(const int_frm_t* frm);

/**
 * idt_init - Initializes the IDT. Registers exception handlers,
 *            IRQ handlers, and syscall handler. Loads the IDT
 *            register.
 */
void __init idt_init(void);
