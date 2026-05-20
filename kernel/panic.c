#include "arch/x86/idt.h"
#include "config.h"
#include "dev/vga.h"
#include "kernel/panic.h"

void __noreturn panic(const char *msg, const char *file, u32 line) {
    vga_clear_screen(VGA_COLOR_BLACK);
    vga_print_string("Kernel panic:", VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_string(msg, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_string("\nFile: ", VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_string(file, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_string("\nLine: ", VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_decimal(line, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    __asm__ volatile ("cli");
    for (;;)
        __asm__ volatile ("hlt");
}