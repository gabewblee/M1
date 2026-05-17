#include "vga.h"

#include "../config.h"

#define _VGA_WIDTH   80
#define _VGA_HEIGHT  25

u8 vga_cur_row = 0;
u8 vga_cur_col = 0;

static void update_cursor(u8 row, u8 col) {
    u16 pos = row * _VGA_WIDTH + col;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (u8) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

static void scroll_up(u8 bcolor) {
    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    u16 blank = (u16)bcolor << 8 | ' ';
    const int penultimate_row = (_VGA_HEIGHT - 1) * _VGA_WIDTH;

    for (int i = 0; i < penultimate_row; i++)
        buf[i] = buf[i + _VGA_WIDTH];

    for (int i = penultimate_row; i < penultimate_row + _VGA_WIDTH; i++)
        buf[i] = blank;
}

static void newline(void) {
    vga_cur_row++;
    vga_cur_col = 0;
    if (vga_cur_row >= _VGA_HEIGHT) {
        scroll_up(VGA_BLACK_COLOR);
        vga_cur_row = _VGA_HEIGHT - 1;
    }
    update_cursor(vga_cur_row, vga_cur_col);
}

static void tab(void) {
    if (vga_cur_col + 4 >= _VGA_WIDTH) {
        newline();
    } else {
        vga_cur_col += 4;
        update_cursor(vga_cur_row, vga_cur_col);
    }
}

static void backspace(void) {
    if (vga_cur_row == 0 && vga_cur_col == 0)
        return;

    if (vga_cur_col > 0) {
        vga_cur_col--;
    } else {
        vga_cur_row--;
        vga_cur_col = _VGA_WIDTH - 1;
    }

    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    u16 blank = (u16)' ' | ((u16)((VGA_BLACK_COLOR << 4) | (VGA_WHITE_COLOR & 0x0F)) << 8);
    buf[vga_cur_row * _VGA_WIDTH + vga_cur_col] = blank;
    update_cursor(vga_cur_row, vga_cur_col);
}

void vga_enable_cursor(u8 start, u8 end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | start);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | end);
}

void vga_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_print_hex(u32 num, u8 fcolor, u8 bcolor) {
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

void vga_print_decimal(u32 num, u8 fcolor, u8 bcolor) {
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

void vga_print_char(char c, u8 fcolor, u8 bcolor) {
    if (c == '\0')
        return;

    if (c == '\n') {
        newline();
        return;
    }

    if (c == '\t') {
        tab();
        return;
    }

    if (c == '\b') {
        backspace();
        return;
    }

    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    u16 cell = (u16)(u8)c | ((u16)((bcolor << 4) | (fcolor & 0x0F)) << 8);
    buf[vga_cur_row * _VGA_WIDTH + vga_cur_col] = cell;
    vga_cur_col++;
    if (vga_cur_col >= _VGA_WIDTH)
        newline();

    update_cursor(vga_cur_row, vga_cur_col);
}

void vga_print_string(const char* str, u8 fcolor, u8 bcolor) {
    while (*str)
        vga_print_char(*str++, fcolor, bcolor);
}

void vga_clear_screen(u8 color) {
    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    for (int i = 0; i < _VGA_WIDTH * _VGA_HEIGHT; i++)
        buf[i] = (u16)color << 8 | ' ';
        
    vga_cur_row = 0;
    vga_cur_col = 0;
    update_cursor(vga_cur_row, vga_cur_col);
}