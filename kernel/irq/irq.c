#include <arch/x86/idt.h>
#include <arch/x86/pic.h>
#include <config.h>
#include <kernel/capability/endpoint.h>
#include <kernel/core/initcall.h>
#include <kernel/irq/irq.h>
#include <stddef.h>
#include <uapi/compiler.h>
#include <uapi/errno.h>

static ntfn_s* bound[IDT_IRQ_CNT];

static void dispatch(ifrm_s* frm) {
    u8 irq = frm->vec - IDT_EXC_CNT;
    if (bound[irq])
        ntfn_signal(bound[irq], 1u << irq);
}

static void __init irq_init(void) {
    for (u8 irq = 0; irq < IDT_IRQ_CNT; irq++)
        bound[irq] = NULL;
}

i32 irq_bind_ntfn(u8 irq, ntfn_s* nt) {
    if (unlikely(irq >= IDT_IRQ_CNT))
        return -(i32)E_INVAL;

    bound[irq] = nt;
    idt_register_handler(PIC_TO_INT(irq), dispatch);
    irq_clear_mask(irq);
    return E_OK;
}

kernel_initcall(irq_init);
