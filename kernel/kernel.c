#include "arch/x86/idt.h"
#include "arch/x86/pic.h"
#include "boot/multiboot.h"
#include "config.h"
#include "dev/console.h"
#include "kernel/ipc.h"
#include "kernel/panic.h"
#include "kernel/sched.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "mm/kheap.h"
#include "mm/page.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

extern u32         magic;
extern phys_addr_t mbi;

void __noreturn kmain(void) {
    console_init();
    console_puts(VGA_FLAG, "Initialized terminal\n");

    idt_init();
    console_puts(VGA_FLAG, "Initialized IDT\n");

    pic_init(0x20, 0x28);
    console_puts(VGA_FLAG, "Initialized PIC\n");

    irq_clear_mask(0);
    irq_clear_mask(1);
    __asm__ volatile ("sti");
    console_puts(VGA_FLAG, "Initialized interrupts\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        PANIC("Error: Invalid multiboot magic number");

    multiboot_info_t* mbinfo = (multiboot_info_t*)(__va(mbi));
    if(!(mbinfo->flags >> 6 & 0x1))
        PANIC("Error: Invalid mmap provided by GRUB bootloader");

    pmm_init(mbinfo);
    console_puts(VGA_FLAG, "Initialized PMM\n");

    vmm_init();
    console_puts(VGA_FLAG, "Initialized VMM\n");

    kheap_init();
    console_puts(VGA_FLAG, "Initialized kernel heap\n");

    sched_init();
    console_puts(VGA_FLAG, "Initialized scheduler\n");

    ipc_init();
    console_puts(VGA_FLAG, "Initialized IPC\n");

    ktask_init();
    console_puts(VGA_FLAG, "Initialized kernel task\n");

    kthread_init();
    console_puts(VGA_FLAG, "Initialized kernel thread\n");

    pmm_free_init_section();
    for (;;);
}