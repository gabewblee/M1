#include <config.h>
#include <dev/evga.h>
#include <mm/page.h>
#include <uapi/io.h>

#define ROWS 25 /* Early VGA row count    */
#define COLS 80 /* Early VGA column count */

static u8 row = 0;
static u8 col = 0;

volatile static u16* buffer = (u16*)__va(EVGA_PHYS_ADDR);

static void __init scroll_up(u8 bcolor) {
    i32 sec_to_last_row = (ROWS - 1) * COLS;
    for (i32 i = 0; i < sec_to_last_row; i++)
        buffer[i] = buffer[i + COLS];
    
    u16 blank = (u16)bcolor << 8 | ' ';
    for (i32 i = sec_to_last_row; i < sec_to_last_row + COLS; i++)
        buffer[i] = blank;
}

static void __init newline(void) {
    row++;
    col = 0;
    if (row >= ROWS) {
        scroll_up(EVGA_BLACK_COLOR);
        row = ROWS - 1;
    }
}

void __init evga_putc(char c) {
    switch (c) {
        case '\0':
            return;
        case '\n':
            newline();
            return;
        default:
            u16 cell = (u16)(u8)c | ((u16)((EVGA_BLACK_COLOR << 4) | (EVGA_WHITE_COLOR & 0x0F)) << 8);
            buffer[row * COLS + col] = cell;

            col++;
            if (col >= COLS)
                newline();

            return;
    }
}

void __init evga_puts(char* str) {
    for (size_t i = 0; str[i]; i++)
        evga_putc(str[i]);
}

void __init evga_write(char* buf, size_t len) {
    for (size_t i = 0; i < len; i++)
        evga_putc(buf[i]);
}

void __init evga_clear(void) {
    for (i32 i = 0; i < COLS * ROWS; i++)
        buffer[i] = (u16)EVGA_BLACK_COLOR << 8 | ' ';
        
    row = col = 0;
}

void __init evga_init(void) {
    evga_clear();
}