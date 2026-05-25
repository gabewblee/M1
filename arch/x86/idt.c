#include <stddef.h>

#include "arch/x86/idt.h"
#include "arch/x86/pic.h"
#include "kernel/panic.h"

#define ISR_STUB_SZ      15                                                /* ISR stub size              */
#define IDT_FLAG_INT     0xE                                               /* Interrupt gate flag        */
#define IDT_FLAG_DPL0    0 << 5                                            /* DPL0 flag                  */
#define IDT_FLAG_DPL3    3 << 5                                            /* DPL3 flag                  */
#define IDT_FLAG_PRESENT 1 << 7                                            /* Present flag               */
#define IDT_ATTR_KERNEL  (IDT_FLAG_PRESENT | IDT_FLAG_DPL0 | IDT_FLAG_INT) /* Kernel interrupt attribute */
#define IDT_ATTR_USER    (IDT_FLAG_PRESENT | IDT_FLAG_DPL3 | IDT_FLAG_INT) /* User interrupt attribute   */

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

__aligned(0x10)
static idt_entry_t        idt[IDT_INT_MAX_CNT];
static idtr_t             idtr;
static int_handler_func_t handlers[IDT_EXC_CNT + IDT_IRQ_CNT];

extern const u8 syscall_stub[];
extern const u8 gdt_start[];
extern const u8 gdt_kernel_code_seg_desc[];
extern const u8 isr_stub_base[];

static inline idt_entry_t set_desc(virt_addr_t handler, u8 attr) {
    return (idt_entry_t) {
        .isr_low    = handler & 0xFFFF,
        .segment    = gdt_kernel_code_seg_desc - gdt_start,
        .reserved   = 0,
        .attributes = attr,
        .isr_high   = handler >> 16,
    };
}

static inline virt_addr_t get_isr_stub(u8 vector) {
    return (virt_addr_t)(isr_stub_base + vector * ISR_STUB_SZ);
}

void idt_register_handler(u8 vector, int_handler_func_t handler) {
    if (unlikely(vector >= (IDT_EXC_CNT + IDT_IRQ_CNT)))
        PANIC("Error: Invalid vector received");

    handlers[vector] = handler;
}

void idt_unregister_handler(u8 vector) {
    if (unlikely(vector >= (IDT_EXC_CNT + IDT_IRQ_CNT)))
        PANIC("Error: Invalid vector received");

    handlers[vector] = NULL;
}

void int_handler(const int_frm_t* frm) {
    u8 vector = frm->int_no;
    int_handler_func_t handler = handlers[vector];
    if (unlikely(!handler))
        return;

    handler(frm);
    if (vector >= IDT_EXC_CNT)
        pic_send_eoi(vector - IDT_EXC_CNT);
}

void __init idt_init(void) {
    idtr.base  = (virt_addr_t)&idt[0];
    idtr.limit = (u16)(sizeof(idt_entry_t) * IDT_INT_MAX_CNT - 1);

    for (u8 vector = 0; vector < (IDT_EXC_CNT + IDT_IRQ_CNT); vector++)
        idt[vector] = set_desc(get_isr_stub(vector), IDT_ATTR_KERNEL);

    idt[0x80] = set_desc((virt_addr_t)syscall_stub, IDT_ATTR_USER);
    __asm__ volatile("lidt %0" : : "m"(idtr));
}
