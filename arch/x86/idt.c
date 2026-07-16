#include <arch/x86/idt.h>
#include <arch/x86/pic.h>
#include <kernel/core/initcall.h>
#include <kernel/core/panic.h>
#include <stddef.h>

#define ISR_STUB_SZ      15     /* ISR stub size       */
#define IDT_INT_FLAG     0xE    /* Interrupt gate flag */
#define IDT_DPL0_FLAG    0 << 5 /* DPL0 flag           */
#define IDT_DPL3_FLAG    3 << 5 /* DPL3 flag           */
#define IDT_PRESENT_FLAG 1 << 7 /* Present flag        */
#define IDT_SYSCALL_VEC  0x80   /* Syscall vector      */

typedef struct idt_entry_s {
    u16 isr_low;    /* Bits 0-15 of interrupt handler address        */
    u16 segment;    /* Code segment selector                         */
    u8  reserved;   /* Reserved field                                */
    u8  attributes; /* Gate type, privilege level, and present flags */
    u16 isr_high;   /* Bits 16-31 of interrupt handler address       */
} __packed idt_entry_s;

typedef struct idtr_s {
    u16 limit; /* IDT size - 1     */
    u32 base;  /* IDT base address */
} __packed idtr_s;

__aligned(0x10)
static idt_entry_s idt[IDT_INT_MAX_CNT];
static ihandler_f  ihandlers[IDT_EXC_CNT + IDT_IRQ_CNT];

extern u8 gdt_start[];
extern u8 gdt_kernel_code_seg_desc[];
extern u8 isr_stub_base[];
extern u8 syscall_stub[];

static inline idt_entry_s set_idt_entry(virt_addr_t ihandler, u8 attr) {
    return (idt_entry_s){
        .isr_low    = ihandler & 0xFFFF,
        .segment    = gdt_kernel_code_seg_desc - gdt_start,
        .reserved   = 0,
        .attributes = attr,
        .isr_high   = ihandler >> 16,
    };
}

static inline virt_addr_t get_isr_stub(u8 vec) {
    return (virt_addr_t)(isr_stub_base + vec * ISR_STUB_SZ);
}

static void __init idt_init(void) {
    /* Initializes IDT register */
    idtr_s idtr = (idtr_s){
        .limit = (u16)(sizeof(idt_entry_s) * IDT_INT_MAX_CNT - 1),
        .base  = (virt_addr_t)idt
    };

    /* Initializes exception and PIC IRQ ihandlers */
    for (u8 vec = 0; vec < IDT_EXC_CNT + IDT_IRQ_CNT; vec++)
        idt[vec] = set_idt_entry(get_isr_stub(vec), IDT_INT_FLAG | IDT_DPL0_FLAG | IDT_PRESENT_FLAG);

    /* Initializes syscall ihandler */
    idt[IDT_SYSCALL_VEC] = set_idt_entry((virt_addr_t)syscall_stub, IDT_INT_FLAG | IDT_DPL3_FLAG | IDT_PRESENT_FLAG);

    /* Loads the IDT */
    __asm__ volatile("lidt %0" : : "m"(idtr));
}

void idt_register_handler(u8 vec, ihandler_f ihandler) {
    if (unlikely(vec >= IDT_EXC_CNT + IDT_IRQ_CNT))
        return;

    ihandlers[vec] = ihandler;
}

void idt_unregister_handler(u8 vec) {
    if (unlikely(vec >= IDT_EXC_CNT + IDT_IRQ_CNT))
        return;

    ihandlers[vec] = NULL;
}

void ihandler(ifrm_s* frm) {
    if (unlikely(!frm))
        return;

    u8 vec = frm->vec;
    if (unlikely(vec >= IDT_EXC_CNT + IDT_IRQ_CNT))
        return;

    ihandler_f ihandler = ihandlers[vec];
    if (unlikely(!ihandler))
        return;

    ihandler(frm);
    if (IDT_EXC_CNT <= vec && vec < IDT_EXC_CNT + IDT_IRQ_CNT)
        pic_send_eoi(vec - IDT_EXC_CNT);
}

arch_initcall(idt_init);