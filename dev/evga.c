#include "config.h"
#include "dev/evga.h"
#include "mm/page.h"
#include "uapi/io.h"

#define EVGA_COL_CNT 80 /* Early VGA column count */
#define EVGA_ROW_CNT 25 /* Early VGA row count    */

u8 evga_cur_row = 0;
u8 evga_cur_col = 0;

static void __init update_cursor(u8 row, u8 col) {
    const u16 pos = row * EVGA_COL_CNT + col;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (u8) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

static void __init scroll_up(u8 bcolor) {
    volatile u16* buf = (u16*)__va(EVGA_PHYS_ADDR);
    const u16 blank = (u16)bcolor << 8 | ' ';
    const int penultimate_row = (EVGA_ROW_CNT - 1) * EVGA_COL_CNT;

    for (int i = 0; i < penultimate_row; i++)
        buf[i] = buf[i + EVGA_COL_CNT];

    for (int i = penultimate_row; i < penultimate_row + EVGA_COL_CNT; i++)
        buf[i] = blank;
}

static void __init newline(void) {
    evga_cur_row++;
    evga_cur_col = 0;
    if (evga_cur_row >= EVGA_ROW_CNT) {
        scroll_up(EVGA_BLACK_COLOR);
        evga_cur_row = EVGA_ROW_CNT - 1;
    }
    update_cursor(evga_cur_row, evga_cur_col);
}

static void __init tab(void) {
    if (evga_cur_col + 4 >= EVGA_COL_CNT) {
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
        evga_cur_col = EVGA_COL_CNT - 1;
    }

    volatile u16* buf = (u16*)__va(EVGA_PHYS_ADDR);
    const u16 blank = (u16)' ' | ((u16)((EVGA_BLACK_COLOR << 4) | (EVGA_WHITE_COLOR & 0x0F)) << 8);
    buf[evga_cur_row * EVGA_COL_CNT + evga_cur_col] = blank;
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
    const u16 cell = (u16)(u8)c | ((u16)((EVGA_BLACK_COLOR << 4) | (EVGA_WHITE_COLOR & 0x0F)) << 8);
    buf[evga_cur_row * EVGA_COL_CNT + evga_cur_col] = cell;
    evga_cur_col++;
    if (evga_cur_col >= EVGA_COL_CNT)
        newline();

    update_cursor(evga_cur_row, evga_cur_col);
}

void __init evga_puts(const char* str) {
    while (*str)
    evga_putc(*str++);
}

void __init evga_write(const char* buf, const size_t len) {
    for (size_t i = 0; i < len; i++)
    evga_putc(buf[i]);
}

void __init evga_clear(void) {
    const u8 color = EVGA_BLACK_COLOR;
    volatile u16* buf = (u16*)__va(EVGA_PHYS_ADDR);
    for (int i = 0; i < EVGA_COL_CNT * EVGA_ROW_CNT; i++)
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