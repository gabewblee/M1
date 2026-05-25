#include "config.h"
#include "dev/evga.h"
#include "io/io.h"
#include "mm/page.h"

#define EVGA_COLS 80 /* EVGA number of columns */
#define EVGA_ROWS 25 /* EVGA number of rows    */

u8 evga_cur_row = 0;
u8 evga_cur_col = 0;

static void __init update_cursor(u8 row, u8 col) {
    u16 pos = row * EVGA_COLS + col;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (u8) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

static void __init scroll_up(u8 bcolor) {
    volatile u16* buf = (u16*)__va(EVGA_PHYS_ADDR);
    u16 blank = (u16)bcolor << 8 | ' ';
    const int penultimate_row = (EVGA_ROWS - 1) * EVGA_COLS;

    for (int i = 0; i < penultimate_row; i++)
        buf[i] = buf[i + EVGA_COLS];

    for (int i = penultimate_row; i < penultimate_row + EVGA_COLS; i++)
        buf[i] = blank;
}

static void __init newline(void) {
    evga_cur_row++;
    evga_cur_col = 0;
    if (evga_cur_row >= EVGA_ROWS) {
        scroll_up(EVGA_COLOR_BLACK);
        evga_cur_row = EVGA_ROWS - 1;
    }
    update_cursor(evga_cur_row, evga_cur_col);
}

static void __init tab(void) {
    if (evga_cur_col + 4 >= EVGA_COLS) {
        newline();
    } else {
        evga_cur_col += 4;
        update_cursor(evga_cur_row, evga_cur_col);
    }
}

static void __init backspace(void) {
    if (evga_cur_row == 0 && evga_cur_col == 0)
        return;

    if (evga_cur_col > 0) {
        evga_cur_col--;
    } else {
        evga_cur_row--;
        evga_cur_col = EVGA_COLS - 1;
    }

    volatile u16* buf = (u16*)__va(EVGA_PHYS_ADDR);
    u16 blank = (u16)' ' | ((u16)((EVGA_COLOR_BLACK << 4) | (EVGA_COLOR_WHITE & 0x0F)) << 8);
    buf[evga_cur_row * EVGA_COLS + evga_cur_col] = blank;
    update_cursor(evga_cur_row, evga_cur_col);
}

void __init evga_putc(const char c) {
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

    volatile u16* buf = (u16*)__va(EVGA_PHYS_ADDR);
    u16 cell = (u16)(u8)c | ((u16)((EVGA_COLOR_BLACK << 4) | (EVGA_COLOR_WHITE & 0x0F)) << 8);
    buf[evga_cur_row * EVGA_COLS + evga_cur_col] = cell;
    evga_cur_col++;
    if (evga_cur_col >= EVGA_COLS)
        newline();

    update_cursor(evga_cur_row, evga_cur_col);
}

void __init evga_puts(const char* str) {
    while (*str)
    evga_putc(*str++);
}

void __init evga_write(const char* str, const size_t len) {
    for (size_t i = 0; i < len; i++)
    evga_putc(str[i]);
}

void __init evga_clear(void) {
    u8 color = EVGA_COLOR_BLACK;
    volatile u16* buf = (u16*)__va(EVGA_PHYS_ADDR);
    for (int i = 0; i < EVGA_COLS * EVGA_ROWS; i++)
        buf[i] = (u16)color << 8 | ' ';
        
    evga_cur_row = 0;
    evga_cur_col = 0;
    update_cursor(evga_cur_row, evga_cur_col);
}

void __init evga_init(void) {
    evga_clear();

    /* Enable hardware cursor */
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}