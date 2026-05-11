#include "idt.h"
#include "panic.h"

#include "../config.h"
#include "../drivers/vga.h" 

void __noreturn panic(const char *msg, const char *file, u32 line) {
    vga_clear_screen(VGA_BLACK_COLOR);
    vga_print_string("Kernel panic:", VGA_WHITE_COLOR, VGA_BLACK_COLOR);
    vga_print_string(msg, VGA_WHITE_COLOR, VGA_BLACK_COLOR);
    vga_print_string("\nFile: ", VGA_WHITE_COLOR, VGA_BLACK_COLOR);
    vga_print_string(file, VGA_WHITE_COLOR, VGA_BLACK_COLOR);
    vga_print_string("\nLine: ", VGA_WHITE_COLOR, VGA_BLACK_COLOR);
    vga_print_decimal(line, VGA_WHITE_COLOR, VGA_BLACK_COLOR);

    __asm__ volatile ("cli");
    for (;;)
        __asm__ volatile ("hlt");
}