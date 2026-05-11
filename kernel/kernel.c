#include "../boot/multiboot.h"
#include "../config.h"
#include "../drivers/vga.h"

void __noreturn kmain(void) {
    vga_clear_screen(VGA_BLACK_COLOR);
    vga_enable_cursor(0, 15);
    vga_print_string("Initialized terminal\n", VGA_WHITE_COLOR, VGA_BLACK_COLOR);
    for(;;);
}