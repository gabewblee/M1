#include <arch/x86/idt.h>
#include <arch/x86/pic.h>
#include <boot/multiboot.h>
#include <config.h>
#include <dev/console.h>
#include <kernel/core/initcall.h>
#include <kernel/core/panic.h>
#include <kernel/core/sched.h>
#include <kernel/core/task.h>
#include <kernel/core/thread.h>
#include <kernel/irq/irq.h>
#include <mm/kheap.h>
#include <mm/page.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

extern u32         magic;
extern phys_addr_t mbi;
extern initcall_f  sinitcall[];
extern initcall_f  einitcall[];

void __init initcalls(void) {
    for (initcall_f* fn = sinitcall; fn < einitcall; fn++)
        (*fn)();
}

void __noreturn kmain(void) {
    console_init();
    console_puts(ALL_FLAG, "[M1] Initialized terminal\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        PANIC("Error: Invalid multiboot magic number");

    multiboot_info_t* mbinfo = (multiboot_info_t*)(__va(mbi));
    if(!(mbinfo->flags >> 6 & 0x1))
        PANIC("Error: Invalid mmap provided by GRUB bootloader");

    initcalls();
    console_puts(ALL_FLAG, "[M1] Initialized subsystems\n");

    irq_clear_mask(0);
    __asm__ volatile ("sti");

    console_unregister(EVGA_FLAG);
    pmm_free_init_section();
    for (;;) sched_yield();
}