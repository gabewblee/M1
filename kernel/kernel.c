#include "idt.h"
#include "panic.h"
#include "pic.h"

#include "../boot/multiboot.h"
#include "../config.h"
#include "../drivers/vga.h"

void __noreturn kmain(void) {
    vga_clear_screen(VGA_BLACK_COLOR);
    vga_enable_cursor(0, 15);
    vga_print_string("Initialized terminal\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    idt_init();
    vga_print_string("Initialized IDT\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    pic_init(0x20, 0x28);
    vga_print_string("Initialized PIC\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    irq_clear_mask(0);
    irq_clear_mask(1);
    __asm__ volatile ("sti");
    vga_print_string("Initialized interrupts\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);
    for(;;);
}