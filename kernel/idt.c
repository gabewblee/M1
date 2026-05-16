#include "idt.h"
#include "panic.h"
#include "pic.h"
#include "task.h"

#include "../drivers/vga.h"
#include "../io/io.h"
#include "../mm/vmm.h"

#define _NUM_EXCEPS 32
#define _NUM_IRQS   16
#define _NUM_DESC   256

typedef struct idt_entry_t {
    u16 isr_low;
    u16 segment;
    u8  reserved;
    u8  attributes;
    u16 isr_high;
} __packed idt_entry_t;

typedef struct idtr_t {
    u16 limit;
    u32 base;
} __packed idtr_t;

typedef void (*irq_handler_func_t)(void);

static void pit_handler(void);
static void dummy_handler(void);

__aligned(0x10)
static idt_entry_t        idt[_NUM_DESC];
static idtr_t             idtr;
static irq_handler_func_t irq_handlers[_NUM_IRQS] = {
    [0]                    = pit_handler,
    [1 ... (_NUM_IRQS - 1)] = dummy_handler
};

extern u32 isr_stub_table[];
extern u32 irq_stub_table[];
extern u32 syscall_stub[];
extern u8  gdt_start[];
extern u8  gdt_kernel_code_seg_desc[];
extern u8  skheap[];
extern u8  ekheap[];

static void pit_handler(void) {
    sched_tick();
}

static void dummy_handler(void) {
    ;
}

void __cold exception_handler(interrupt_frm_t* frm) {
    if (likely(frm->int_no == 14 && !(frm->err_code & 1))) {
        virt_addr_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        if (likely(cr2 >= (virt_addr_t)skheap && cr2 < (virt_addr_t)ekheap)) {
            vmm_demand_map(cr2 & ~(PG_SZ - 1u));
            return;
        }
    }
    PANIC("Error: CPU exception thrown");
}

void __hot irq_handler(interrupt_frm_t* frm) {
    u8 irq = frm->int_no - _NUM_EXCEPS;
    if (unlikely(irq >= _NUM_IRQS))
        PANIC("Error: Invalid IRQ received");

    pic_send_eoi(irq);
    irq_handlers[irq]();
}

static void set_idt_desc(u8 vector, virt_addr_t isr, u8 flags) {
    idt_entry_t* descriptor = &idt[vector];
    descriptor->isr_low     = isr & 0xFFFF;
    descriptor->segment     = gdt_kernel_code_seg_desc - gdt_start;
    descriptor->reserved    = 0;
    descriptor->attributes  = flags;
    descriptor->isr_high    = isr >> 16;
}

void __init idt_init(void) {
    idtr.base  = (virt_addr_t)&idt[0];
    idtr.limit = (u16)(sizeof(idt_entry_t) * _NUM_DESC - 1);
    for (u8 vector = 0; vector < _NUM_EXCEPS; vector++)
        set_idt_desc(vector, isr_stub_table[vector], 0x8E);

    for (u8 vector = 0; vector < _NUM_IRQS; vector++)
        set_idt_desc(vector + _NUM_EXCEPS, irq_stub_table[vector], 0x8E);

    set_idt_desc(0x80, (virt_addr_t)syscall_stub, 0xEE);
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}