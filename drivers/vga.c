#include "vga.h"

#include "../config.h"

#define VGA_WIDTH  80 /* VGA text mode screen width */
#define VGA_HEIGHT 25 /* VGA text mode screen height */

u8 vga_cur_row = 0;   /* Current cursor row */
u8 vga_cur_col = 0;   /* Current cursor column */

static void vga_update_cursor(u8 row, u8 col) {
    u16 pos = row * VGA_WIDTH + col;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (u8) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

static void vga_scroll_up(u8 bcolor) {
    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    u16 blank = (u16)bcolor << 8 | ' ';
    const int penultimate_row = (VGA_HEIGHT - 1) * VGA_WIDTH;

    for (int i = 0; i < penultimate_row; i++)
        buf[i] = buf[i + VGA_WIDTH];

    for (int i = penultimate_row; i < penultimate_row + VGA_WIDTH; i++)
        buf[i] = blank;
}

static void vga_newline(void) {
    vga_cur_row++;
    vga_cur_col = 0;
    if (vga_cur_row >= VGA_HEIGHT) {
        vga_scroll_up(VGA_BLACK_COLOR);
        vga_cur_row = VGA_HEIGHT - 1;
    }
    vga_update_cursor(vga_cur_row, vga_cur_col);
}

static void vga_tab(void) {
    if (vga_cur_col + 4 >= VGA_WIDTH) {
        vga_newline();
    } else {
        vga_cur_col += 4;
        vga_update_cursor(vga_cur_row, vga_cur_col);
    }
}

static void vga_backspace(void) {
    if (vga_cur_row == 0 && vga_cur_col == 0)
        return;

    if (vga_cur_col > 0) {
        vga_cur_col--;
    } else {
        vga_cur_row--;
        vga_cur_col = VGA_WIDTH - 1;
    }

    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    u16 blank = (u16)' ' | ((u16)((VGA_BLACK_COLOR << 4) | (VGA_WHITE_COLOR & 0x0F)) << 8);
    buf[vga_cur_row * VGA_WIDTH + vga_cur_col] = blank;
    vga_update_cursor(vga_cur_row, vga_cur_col);
}

void __cold vga_enable_cursor(u8 start, u8 end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | start);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | end);
}

void __cold vga_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void __hot vga_print_hex(u32 num, u8 fcolor, u8 bcolor) {
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];
    int i = 10;
    
    buffer[i--] = '\0';
    
    if (num == 0) {
        vga_print_string("0x0", fcolor, bcolor);
        return;
    }
    
    while (num > 0) {
        buffer[i--] = hex_chars[num & 0xF];
        num >>= 4;
    }
    
    buffer[i--] = 'x';
    buffer[i] = '0';
    
    vga_print_string(&buffer[i], fcolor, bcolor);
}

void __hot vga_print_decimal(u32 num, u8 fcolor, u8 bcolor) {
    char buffer[21];
    int i = 20;
    
    buffer[i--] = '\0';
    
    if (num == 0) {
        vga_print_char('0', fcolor, bcolor);
        return;
    }
    
    while (num > 0) {
        u64 q = num / 10;
        u64 r = num - (q * 10);
        buffer[i--] = '0' + r;
        num = q;
    }
    
    vga_print_string(&buffer[i + 1], fcolor, bcolor);
}

void __hot vga_print_char(char c, u8 fcolor, u8 bcolor) {
    if (c == '\0')
        return;

    if (c == '\n') {
        vga_newline();
        return;
    }

    if (c == '\t') {
        vga_tab();
        return;
    }

    if (c == '\b') {
        vga_backspace();
        return;
    }

    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    u16 cell = (u16)(u8)c | ((u16)((bcolor << 4) | (fcolor & 0x0F)) << 8);
    buf[vga_cur_row * VGA_WIDTH + vga_cur_col] = cell;
    vga_cur_col++;
    if (vga_cur_col >= VGA_WIDTH)
        vga_newline();

    vga_update_cursor(vga_cur_row, vga_cur_col);
}

void __hot vga_print_string(const char* str, u8 fcolor, u8 bcolor) {
    while (*str)
        vga_print_char(*str++, fcolor, bcolor);
}

void __cold vga_clear_screen(u8 color) {
    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        buf[i] = (u16)color << 8 | ' ';
        
    vga_cur_row = 0;
    vga_cur_col = 0;
    vga_update_cursor(vga_cur_row, vga_cur_col);
}