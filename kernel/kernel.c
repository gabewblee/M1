#include "arch/x86/idt.h"
#include "arch/x86/pic.h"
#include "boot/multiboot.h"
#include "config.h"
#include "dev/vga.h"
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
    vga_clear_screen(VGA_COLOR_BLACK);
    vga_enable_cursor(0, 15);
    vga_print_string("Initialized terminal\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    idt_init();
    vga_print_string("Initialized IDT\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    pic_init(0x20, 0x28);
    vga_print_string("Initialized PIC\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    irq_clear_mask(0);
    irq_clear_mask(1);
    __asm__ volatile ("sti");
    vga_print_string("Initialized interrupts\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        PANIC("Error: Invalid multiboot magic number");

    multiboot_info_t* mbinfo = (multiboot_info_t*)(__va(mbi));
    if(!(mbinfo->flags >> 6 & 0x1))
        PANIC("Error: Invalid mmap provided by GRUB bootloader");

    pmm_init(mbinfo);
    vga_print_string("Initialized PMM\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vmm_init();
    vga_print_string("Initialized VMM\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    kheap_init();
    vga_print_string("Initialized kernel heap\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    sched_init();
    vga_print_string("Initialized scheduler\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    ipc_init();
    vga_print_string("Initialized IPC\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    ktask_init();
    vga_print_string("Initialized kernel task\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    kthread_init();
    vga_print_string("Initialized kernel thread\n", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    pmm_free_init_section();
    for (;;);
}