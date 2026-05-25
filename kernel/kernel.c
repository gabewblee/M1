#include "arch/x86/idt.h"
#include "arch/x86/pic.h"
#include "boot/multiboot.h"
#include "config.h"
#include "dev/console.h"
#include "kernel/ipc.h"
#include "kernel/panic.h"
#include "kernel/sched.h"
#include "kernel/servers.h"
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
    console_puts(ALL_FLAG, "[M1] Initialized terminal\n");

    idt_init();
    console_puts(ALL_FLAG, "[M1] Initialized IDT\n");

    pic_init(0x20, 0x28);
    console_puts(ALL_FLAG, "[M1] Initialized PIC\n");

    sched_init();
    console_puts(ALL_FLAG, "[M1] Initialized scheduler\n");

    irq_clear_mask(0);
    __asm__ volatile ("sti");
    console_puts(ALL_FLAG, "[M1] Initialized interrupts\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        PANIC("Error: Invalid multiboot magic number");

    const multiboot_info_t* mbinfo = (const multiboot_info_t*)(__va(mbi));
    if(!(mbinfo->flags >> 6 & 0x1))
        PANIC("Error: Invalid mmap provided by GRUB bootloader");

    pmm_init(mbinfo);
    console_puts(ALL_FLAG, "[M1] Initialized PMM\n");

    vmm_init();
    console_puts(ALL_FLAG, "[M1] Initialized VMM\n");

    kheap_init();
    console_puts(ALL_FLAG, "[M1] Initialized kernel heap\n");

    ipc_init();
    console_puts(ALL_FLAG, "[M1] Initialized IPC\n");

    task0_init();
    console_puts(ALL_FLAG, "[M1] Initialized task0\n");

    thread0_init();
    console_puts(ALL_FLAG, "[M1] Initialized thread0\n");

    servers_init();
    console_unregister_dev(EVGA_FLAG);

    pmm_free_init_section();
    for (;;) sched_yield();
}