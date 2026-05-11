#pragma once

#include <stdint.h>

#include "../config.h"

typedef struct {
    u32         edi, esi, ebp, esp, ebx, edx, ecx, eax; /* General purpose registers */
    u32         int_no;                                 /* Interrupt number */
    u32         err_code;                               /* Error code */
    virt_addr_t eip;                                    /* Instruction pointer */
    u32         cs, eflags;                             /* Kernel code segment and flags */
} interrupt_frame_t;

/**
 * exception_handler - Exception handler
 * @frame: The pointer to the interrupt stack frame
 */
void __cold exception_handler(interrupt_frame_t* frame);

/**
 * irq_handler - Interrupt handler
 * @frame: The pointer to the interrupt stack frame
 */
void __hot irq_handler(interrupt_frame_t* frame);

/**
 * idt_init - Initialize the IDT
 */
void __init idt_init(void);