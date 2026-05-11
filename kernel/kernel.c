#include "gdt.h"
#include "idt.h"
#include "panic.h"
#include "pic.h"
#include "task.h"

#include "../boot/multiboot.h"
#include "../config.h"
#include "../drivers/vga.h"
#include "../mm/kheap.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"

extern u32         magic; /* Multiboot magic number */
extern phys_addr_t mbi;   /* Multiboot information structure address */

void __noreturn kmain(void) {
    vga_clear_screen(VGA_BLACK_COLOR);
    vga_enable_cursor(0, 15);
    vga_print_string("Initialized terminal\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    gdt_init();
    vga_print_string("Initialized GDT\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    idt_init();
    vga_print_string("Initialized IDT\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    pic_init(0x20, 0x28);
    vga_print_string("Initialized PIC\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    irq_clear_mask(0);
    irq_clear_mask(1);
    __asm__ volatile ("sti");
    vga_print_string("Initialized interrupts\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        PANIC("Error: Invalid multiboot magic number");

    multiboot_info_t* mbinfo = (multiboot_info_t*)(__va(mbi));
    if(!(mbinfo->flags >> 6 & 0x1))
        PANIC("Error: Invalid mmap provided by GRUB bootloader");

    pmm_init(mbinfo);
    vga_print_string("Initialized PMM\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    vmm_init();
    vga_print_string("Initialized VMM\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    kheap_init();
    vga_print_string("Initialized kernel heap\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    multitasking_init();
    vga_print_string("Initialized multitasking\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    pmm_free_init_section();
    for(;;);
}