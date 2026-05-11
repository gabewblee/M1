#pragma once

#include <stdint.h>

#include "../config.h"

#define PIC1		    0x20	 /* IO base address for master PIC */
#define PIC2		    0xA0	 /* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	    (PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	    (PIC2+1)

#define PIC_EOI         0x20     /* EOI Code */

#define ICW1_ICW4	    0x01	 /* Indicates that ICW4 will be present */
#define ICW1_SINGLE	    0x02	 /* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04	 /* Call address interval 4 (8) */
#define ICW1_LEVEL	    0x08	 /* Level triggered (edge) mode */
#define ICW1_INIT	    0x10	 /* Initialization - required! */

#define ICW4_8086	    0x01	 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	    0x02	 /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08	 /* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C	 /* Buffered mode/master */
#define ICW4_SFNM	    0x10	 /* Special fully nested (not) */

#define CASCADE_IRQ     2        /* IRQ number for cascading */

/* OCW3 commands for reading PIC registers */
#define PIC_READ_IRR    0x0a     /* Read IRQ Request Register */
#define PIC_READ_ISR    0x0b     /* Read In-Service Register */

/**
 * pic_send_eoi - Send EOI signal to PIC
 * @irq: The IRQ number that was handled
 */
void pic_send_eoi(u8 irq);

/**
 * pic_remap - Remap PIC IRQ vectors
 * @offset1: The vector offset for master PIC
 * @offset2: The vector offset for slave PIC
 */
void pic_remap(int offset1, int offset2);

/**
 * pic_disable - Disable the 8259 PIC
 */
void pic_disable(void);

/**
 * irq_set_mask - Mask a specific IRQ
 * @irq: The IRQ number to mask
 */
void irq_set_mask(u8 irq);

/**
 * irq_clear_mask - Unmask a specific IRQ
 * @irq: The IRQ number to unmask
 */
void irq_clear_mask(u8 irq);

/**
 * pic_get_irr - Get combined value of IRQ request register
 * @return: The combined IRR from both PICs (bits 0-7 master, 8-15 slave)
 */
u16 pic_get_irr(void);

/**
 * pic_get_isr - Get combined value of IRQ in-service register
 * @return: The combined ISR from both PICs (bits 0-7 master, 8-15 slave)
 */
u16 pic_get_isr(void);

/**
 * pic_init - Initialize the 8259 PIC
 * @offset1: The vector offset for master PIC (IRQ 0-7)
 * @offset2: The vector offset for slave PIC (IRQ 8-15)
 */
void __init pic_init(u8 offset1, u8 offset2);