#include <arch/x86/idt.h>
#include <arch/x86/pic.h>
#include <config.h>
#include <kernel/capability/endpoint.h>
#include <kernel/core/initcall.h>
#include <kernel/irq/irq.h>
#include <stddef.h>
#include <uapi/compiler.h>
#include <uapi/errno.h>

static notification_s* bound[IDT_IRQ_CNT];

static void dispatch(ifrm_s* frm) {
    u8 irq = frm->vec - IDT_EXC_CNT;
    if (bound[irq])
        notification_signal(bound[irq], 1u << irq);
}

i32 irq_bind_notification(u8 irq, notification_s* nt) {
    if (unlikely(irq >= IDT_IRQ_CNT))
        return -(i32)E_INVAL;

    bound[irq] = nt;
    idt_register_handler(PIC_TO_INT(irq), dispatch);
    irq_clear_mask(irq);
    return E_OK;
}

void __init irq_init(void) {
    for (u8 irq = 0; irq < IDT_IRQ_CNT; irq++)
        bound[irq] = NULL;
}

kernel_initcall(irq_init);
