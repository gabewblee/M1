#include <arch/x86/idt.h>
#include <arch/x86/pic.h>
#include <config.h>
#include <kernel/core/initcall.h>
#include <kernel/core/sched.h>
#include <kernel/core/thread.h>
#include <kernel/irq/irq.h>
#include <libk/list.h>
#include <uapi/compiler.h>
#include <uapi/errno.h>
#include <uapi/syscall.h>

static list_node_s waiters[IDT_IRQ_CNT];

static void dispatch(int_frm_s* frm) {
    u8 irq = frm->vec - IDT_EXC_CNT;
    while (!list_is_empty(&waiters[irq])) {
        thread_ctrl_blk_s* thread = list_first_entry(
            &waiters[irq], thread_ctrl_blk_s, wait_link
        );

        sched_unblock(thread);
    }
}

i32 irq_register_handler(u8 irq) {
    if (unlikely(irq >= IDT_IRQ_CNT))
        return -(i32)E_INVAL;

    idt_register_handler(PIC_TO_INT(irq), dispatch);
    irq_clear_mask(irq);
    return E_OK;
}

i32 irq_wait_for(u8 irq) {
    if (unlikely(irq >= IDT_IRQ_CNT))
        return -(i32)E_INVAL;

    sched_block(&waiters[irq], THREAD_STATE_BLOCKED);
    return E_OK;
}

void __init irq_init(void) {
    for (u8 irq = 0; irq < IDT_IRQ_CNT; irq++)
        list_init(&waiters[irq]);
}

subsys_initcall(irq_init);