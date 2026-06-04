#include <stddef.h>

#include "arch/x86/idt.h"
#include "arch/x86/pic.h"
#include "kernel/core/panic.h"

#define ISR_STUB_SZ      15     /* ISR stub size    */
#define IDT_VEC_FLAG     0xE    /* Vector gate flag */
#define IDT_DPL0_FLAG    0 << 5 /* DPL0 flag        */
#define IDT_DPL3_FLAG    3 << 5 /* DPL3 flag        */
#define IDT_PRESENT_FLAG 1 << 7 /* Present flag     */

typedef struct idt_entry_s {
    u16 isr_low;    /* Bits 0-15 of interrupt handler address       */
    u16 segment;    /* Code segment selector                        */
    u8  reserved;   /* Reserved field                               */
    u8  attributes; /* Gate type, privilege level, and present flag */
    u16 isr_high;   /* Bits 16-31 of interrupt handler address      */
} __packed idt_entry_s;

typedef struct idtr_s {
    u16 limit; /* IDT size - 1     */
    u32 base;  /* IDT base address */
} __packed idtr_s;

__aligned(0x10)
static idt_entry_s        idt[IDT_VEC_MAX_CNT];
static idtr_s             idtr;
static int_handler_func_t handlers[IDT_EXC_CNT + IDT_IRQ_CNT];

extern const u8 gdt_start[];
extern const u8 gdt_kernel_code_seg_desc[];
extern const u8 isr_stub_base[];
extern const u8 syscall_stub[];

static inline idt_entry_s set_desc(virt_addr_t handler, const u8 attr) {
    return (idt_entry_s) {
        .isr_low    = handler & 0xFFFF,
        .segment    = gdt_kernel_code_seg_desc - gdt_start,
        .reserved   = 0,
        .attributes = attr,
        .isr_high   = handler >> 16,
    };
}

static inline virt_addr_t get_isr_stub(u8 vec) {
    return (virt_addr_t)(isr_stub_base + vec * ISR_STUB_SZ);
}

void idt_register_handler(u8 vec, int_handler_func_t handler) {
    handlers[vec] = handler;
}

void idt_unregister_handler(u8 vec) {
    handlers[vec] = NULL;
}

void int_handler(const int_frm_s* frm) {
    if (unlikely(!frm || frm->vec >= IDT_EXC_CNT + IDT_IRQ_CNT))
        return;

    u8 vec = frm->vec;
    int_handler_func_t handler = handlers[vec];
    if (unlikely(!handler))
        return;

    handler(frm);
    if (vec >= IDT_EXC_CNT)
        pic_send_eoi(vec - IDT_EXC_CNT);
}

void __init idt_init(void) {
    idtr.base  = (virt_addr_t)idt;
    idtr.limit = (u16)(sizeof(idt_entry_s) * IDT_VEC_MAX_CNT - 1);
    for (u8 vec = 0; vec < IDT_EXC_CNT + IDT_IRQ_CNT; vec++)
        idt[vec] = set_desc(get_isr_stub(vec), IDT_VEC_FLAG | IDT_DPL0_FLAG | IDT_PRESENT_FLAG);

    idt[0x80] = set_desc((virt_addr_t)syscall_stub, IDT_VEC_FLAG | IDT_DPL3_FLAG | IDT_PRESENT_FLAG);
    __asm__ volatile("lidt %0" : : "m"(idtr));
}