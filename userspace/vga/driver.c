#include <uapi/io.h>
#include <userspace/vga/driver.h>

u8 vga_cur_row;
u8 vga_cur_col;

static void update_cursor(u8 row, u8 col) {
    u16 pos = row * VGA_COL_CNT + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

static void scroll_up(u8 bcolor) {
    volatile u16* buf = (u16*)VGA_ADDR;
    u16 blank = (u16)bcolor << 8 | ' ';
    int penultimate_row = (VGA_ROW_CNT - 1) * VGA_COL_CNT;
    for (int i = 0; i < penultimate_row; i++)
        buf[i] = buf[i + VGA_COL_CNT];

    for (int i = penultimate_row; i < penultimate_row + VGA_COL_CNT; i++)
        buf[i] = blank;
}

static void newline(void) {
    vga_cur_row++;
    vga_cur_col = 0;
    if (vga_cur_row >= VGA_ROW_CNT) {
        scroll_up(VGA_BLACK_COLOR);
        vga_cur_row = VGA_ROW_CNT - 1;
    }
    update_cursor(vga_cur_row, vga_cur_col);
}

static void tab(void) {
    if (vga_cur_col + 4 >= VGA_COL_CNT) {
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
        vga_cur_col = VGA_COL_CNT - 1;
    }

    volatile u16* buf = (u16*)VGA_ADDR;
    u16 blank = (u16)' ' | ((u16)((VGA_BLACK_COLOR << 4) | (VGA_WHITE_COLOR & 0x0F)) << 8);
    buf[vga_cur_row * VGA_COL_CNT + vga_cur_col] = blank;
    update_cursor(vga_cur_row, vga_cur_col);
}

void vga_putc(char c) {
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

    volatile u16* buf = (u16*)VGA_ADDR;
    u16 cell = (u16)(u8)c | ((u16)((VGA_BLACK_COLOR << 4) | (VGA_WHITE_COLOR & 0x0F)) << 8);
    buf[vga_cur_row * VGA_COL_CNT + vga_cur_col] = cell;
    vga_cur_col++;
    if (vga_cur_col >= VGA_COL_CNT)
        newline();

    update_cursor(vga_cur_row, vga_cur_col);
}

void vga_puts(char* str) {
    while (*str)
        vga_putc(*str++);
}

void vga_write(char* str, size_t len) {
    for (size_t i = 0; i < len; i++)
        vga_putc(str[i]);
}

void vga_clear(void) {
    volatile u16* buf = (u16*)VGA_ADDR;
    for (int i = 0; i < VGA_COL_CNT * VGA_ROW_CNT; i++)
        buf[i] = (u16)VGA_BLACK_COLOR << 8 | ' ';

    vga_cur_row = 0;
    vga_cur_col = 0;
    update_cursor(vga_cur_row, vga_cur_col);
}

void vga_init(void) {
    vga_clear();

    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}