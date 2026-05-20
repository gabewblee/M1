#include "arch/x86/idt.h"
#include "arch/x86/pic.h"
#include "io/io.h"
#include "kernel/panic.h"
#include "kernel/sched.h"
#include "mm/vmm.h"

typedef struct idt_entry_t {
    u16 isr_low;    /* Bits 0-15 of interrupt handler address       */
    u16 segment;    /* Code segment selector                        */
    u8  reserved;   /* Reserved field                               */
    u8  attributes; /* Gate type, privilege level, and present flag */
    u16 isr_high;   /* Bits 16-31 of interrupt handler address      */
} __packed idt_entry_t;

typedef struct idtr_t {
    u16 limit; /* IDT size - 1     */
    u32 base;  /* IDT base address */
} __packed idtr_t;

typedef void (*irq_handler_func_t)(void);

static void pit_handler(void);
static void dummy_handler(void);

__aligned(0x10)
static idt_entry_t        idt[IDT_DESC_CNT];
static idtr_t             idtr;
static irq_handler_func_t irq_handlers[IDT_IRQ_CNT] = {
    [0]                    = pit_handler,
    [1 ... (IDT_IRQ_CNT - 1)] = dummy_handler
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

void exception_handler(interrupt_frm_t* frm) {
    if (likely(frm->int_no == 14 && !(frm->err_code & 1))) {
        virt_addr_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        if (likely(cr2 >= (virt_addr_t)skheap && cr2 < (virt_addr_t)ekheap)) {
            vmm_demand_map(cr2 & ~(PG_SZ - 1u), PG_FLAG_RW);
            return;
        }
    }

    PANIC("Error: CPU exception thrown");
}

void irq_handler(interrupt_frm_t* frm) {
    u8 irq = frm->int_no - IDT_EXCEPTION_CNT;
    if (unlikely(irq >= IDT_IRQ_CNT))
        PANIC("Error: Invalid IRQ received");

    pic_send_eoi(irq);
    irq_handlers[irq]();
}

static void set_idt_desc(u8 vector, virt_addr_t isr, u8 flags) {
    idt_entry_t* desc = &idt[vector];
    desc->isr_low     = isr & 0xFFFF;
    desc->segment     = gdt_kernel_code_seg_desc - gdt_start;
    desc->reserved    = 0;
    desc->attributes  = flags;
    desc->isr_high    = isr >> 16;
}

void __init idt_init(void) {
    idtr.base  = (virt_addr_t)&idt[0];
    idtr.limit = (u16)(sizeof(idt_entry_t) * IDT_DESC_CNT - 1);
    for (u8 vector = 0; vector < IDT_EXCEPTION_CNT; vector++)
        set_idt_desc(vector, isr_stub_table[vector], 0x8E);

    for (u8 vector = 0; vector < IDT_IRQ_CNT; vector++)
        set_idt_desc(vector + IDT_EXCEPTION_CNT, irq_stub_table[vector], 0x8E);

    set_idt_desc(0x80, (virt_addr_t)syscall_stub, 0xEE);
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}