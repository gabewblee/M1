#include "idt.h"
#include "panic.h"
#include "pic.h"

#include "../drivers/vga.h"
#include "../io/io.h"

#define IDT_NUM_EXCEPTIONS  32                           /* Number of exceptions */
#define IDT_NUM_IRQS        16                           /* Number of IRQs */
#define IDT_NUM_DESCRIPTORS 256                          /* Number of IDT descriptors */

typedef struct idt_entry_t {
    u16 isr_low;                                         /* Lower 16 bits of ISR address */
    u16 segment;                                         /* KCS selector */
    u8  reserved;                                        /* Set to 0 */
    u8  attributes;                                      /* Attributes */
    u16 isr_high;                                        /* Upper 16 bits of ISR address */
} __packed idt_entry_t;

typedef struct idtr_t {
    u16 limit;                                           /* Size of IDT - 1 */
    u32 base;                                            /* IDT pointer */
} __packed idtr_t;

typedef void (*irq_handler_func_t)(void);                /* IRQ handler function pointer type */
static void irq_dummy_handler(void);

__aligned(0x10)
static idt_entry_t        idt[IDT_NUM_DESCRIPTORS];      /* Interrupt Descriptor Table */
static idtr_t             idtr;                          /* Interrupt Descriptor Table Register */
static irq_handler_func_t irq_handlers[IDT_NUM_IRQS] = { /* Array of IRQ handler function pointers */
    [0 ... (IDT_NUM_IRQS - 1)] = irq_dummy_handler
};

extern u32                isr_stub_table[];              /* ISR stub virtual addresses */
extern u32                irq_stub_table[];              /* IRQ stub virtual addresses */
extern u8                 gdt_start[];                   /* Start of the GDT in virtual memory */
extern u8                 gdt_kernel_code_seg_desc[];    /* Start of the KCS descriptor in virtual memory */

static void irq_dummy_handler(void) {
    ;
}

static void idt_set_desc(u8 vector, virt_addr_t isr, u8 flags) {
    idt_entry_t* descriptor = &idt[vector];
    descriptor->isr_low     = isr & 0xFFFF;
    descriptor->segment     = gdt_kernel_code_seg_desc - gdt_start;
    descriptor->reserved    = 0;
    descriptor->attributes  = flags;
    descriptor->isr_high    = isr >> 16;
}

void __cold exception_handler(interrupt_frame_t* frame) {
    PANIC("Error: CPU exception thrown");
}

void __hot irq_handler(interrupt_frame_t* frame) {
    u8 irq = frame->int_no - IDT_NUM_EXCEPTIONS;
    if (irq >= IDT_NUM_IRQS)
        PANIC("Error: Invalid IRQ received");

    pic_send_eoi(irq);
    irq_handlers[irq]();
}

void __init idt_init(void) {
    idtr.base  = (virt_addr_t)&idt[0];
    idtr.limit = (u16)(sizeof(idt_entry_t) * IDT_NUM_DESCRIPTORS - 1);
    for (u8 vector = 0; vector < IDT_NUM_EXCEPTIONS; vector++)
        idt_set_desc(vector, isr_stub_table[vector], 0x8E);

    for (u8 vector = 0; vector < IDT_NUM_IRQS; vector++)
        idt_set_desc(vector + IDT_NUM_EXCEPTIONS, irq_stub_table[vector], 0x8E);

    __asm__ volatile ("lidt %0" : : "m"(idtr));
}