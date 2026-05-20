#include "config.h"
#include "dev/vga.h"
#include "io/io.h"
#include "mm/page.h"

#define VGA_COLS 80 /* VGA number of columns */
#define VGA_ROWS 25 /* VGA number of rows    */

u8 vga_cur_row = 0;
u8 vga_cur_col = 0;

/* Forward declarations */
static void print_char(char c, u8 fcolor, u8 bcolor);
static void print_string(const char* str, u8 fcolor, u8 bcolor);

static void update_cursor(u8 row, u8 col) {
    u16 pos = row * VGA_COLS + col;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (u8) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

static void scroll_up(u8 bcolor) {
    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    u16 blank = (u16)bcolor << 8 | ' ';
    const int penultimate_row = (VGA_ROWS - 1) * VGA_COLS;

    for (int i = 0; i < penultimate_row; i++)
        buf[i] = buf[i + VGA_COLS];

    for (int i = penultimate_row; i < penultimate_row + VGA_COLS; i++)
        buf[i] = blank;
}

static void newline(void) {
    vga_cur_row++;
    vga_cur_col = 0;
    if (vga_cur_row >= VGA_ROWS) {
        scroll_up(VGA_COLOR_BLACK);
        vga_cur_row = VGA_ROWS - 1;
    }
    update_cursor(vga_cur_row, vga_cur_col);
}

static void tab(void) {
    if (vga_cur_col + 4 >= VGA_COLS) {
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
        vga_cur_col = VGA_COLS - 1;
    }

    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    u16 blank = (u16)' ' | ((u16)((VGA_COLOR_BLACK << 4) | (VGA_COLOR_WHITE & 0x0F)) << 8);
    buf[vga_cur_row * VGA_COLS + vga_cur_col] = blank;
    update_cursor(vga_cur_row, vga_cur_col);
}

void vga_putc(const char c) {
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
    u16 cell = (u16)(u8)c | ((u16)((VGA_COLOR_BLACK << 4) | (VGA_COLOR_WHITE & 0x0F)) << 8);
    buf[vga_cur_row * VGA_COLS + vga_cur_col] = cell;
    vga_cur_col++;
    if (vga_cur_col >= VGA_COLS)
        newline();

    update_cursor(vga_cur_row, vga_cur_col);
}

void vga_puts(const char* str) {
    while (*str)
        vga_putc(*str++);
}

void vga_write(const char* str, const size_t len) {
    for (size_t i = 0; i < len; i++)
        vga_putc(str[i]);
}

void vga_clear(void) {
    u8 color = VGA_COLOR_BLACK;
    volatile u16* buf = (u16*)__va(VGA_PHYS_ADDR);
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        buf[i] = (u16)color << 8 | ' ';
        
    vga_cur_row = 0;
    vga_cur_col = 0;
    update_cursor(vga_cur_row, vga_cur_col);
}

void vga_init(void) {
    vga_clear();

    /* Enable hardware cursor */
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}