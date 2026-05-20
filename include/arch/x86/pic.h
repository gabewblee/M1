#pragma once

#include <stdint.h>

#include "config.h"

#define PIC1         0x20       /* Master PIC IO base address */
#define PIC2         0xA0       /* Slave PIC IO base address  */
#define PIC1_COMMAND PIC1       /* Master PIC command port    */
#define PIC1_DATA    (PIC1 + 1) /* Master PIC data port       */
#define PIC2_COMMAND PIC2       /* Slave PIC command port     */
#define PIC2_DATA    (PIC2 + 1) /* Slave PIC data port        */

#define PIC_EOI      0x20       /* End-of-interrupt code      */

#define ICW1_ICW4    0x01       /* ICW4 present               */
#define ICW1_SINGLE  0x02       /* Single mode                */
#define ICW1_INT4    0x04       /* Call address interval 4    */
#define ICW1_LEVEL   0x08       /* Level triggered mode       */
#define ICW1_INIT    0x10       /* Initialization required    */

#define ICW4_8086    0x01       /* 8086/88 mode               */
#define ICW4_AUTO    0x02       /* Auto EOI                   */
#define ICW4_BUF_SLV 0x08       /* Buffered mode/slave        */
#define ICW4_BUF_MST 0x0C       /* Buffered mode/master       */
#define ICW4_SFNM    0x10       /* Special fully nested mode  */

#define CASCADE_IRQ  2          /* Cascade IRQ number         */

#define PIC_READ_IRR 0x0a       /* Read IRR command           */
#define PIC_READ_ISR 0x0b       /* Read ISR command           */

/**
 * pic_send_eoi - Sends an end-of-interrupt signal to the PIC.
 * @irq: The IRQ number that was handled.
 */
void pic_send_eoi(u8 irq);

/**
 * pic_remap - Remaps PIC IRQ vectors to the given offsets. After remapping, 
 *             IRQs 0-7 map to @offset1 to @offset1 + 7 and IRQs 8-15 to 
 *             @offset2 to @offset2 + 7. All IRQ masks are cleared.
 * @offset1: The vector offset for the master PIC.
 * @offset2: The vector offset for the slave PIC.
 */
void pic_remap(int offset1, int offset2);

/**
 * pic_disable - Disables the 8259 PIC by masking all IRQ lines.
 */
void pic_disable(void);

/**
 * irq_set_mask - Masks a specific IRQ line.
 * @irq: The IRQ number to mask.
 */
void irq_set_mask(u8 irq);

/**
 * irq_clear_mask - Unmasks a specific IRQ line.
 * @irq: The IRQ number to unmask.
 */
void irq_clear_mask(u8 irq);

/**
 * pic_get_irr - Returns the combined IRQ request register from both PICs.
 * @return: The combined IRR value; bits 0-7 master, bits 8-15 slave.
 */
u16 pic_get_irr(void);

/**
 * pic_get_isr - Returns the combined IRQ in-service register from both PICs.
 * @return: The combined ISR value; bits 0-7 master, bits 8-15 slave.
 */
u16 pic_get_isr(void);

/**
 * pic_init - Initializes and unmasks the 8259 PIC with the given vector offsets.
 * @offset1: The vector offset for the master PIC, applied to IRQ 0-7.
 * @offset2: The vector offset for the slave PIC, applied to IRQ 8-15.
 */
void __init pic_init(u8 offset1, u8 offset2);