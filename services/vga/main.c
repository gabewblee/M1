#include "../../include/types.h"
#include "../lib/syscall.h"

#define VGA_TEXT_COLS        80u              /* Text mode column count           */
#define VGA_TEXT_ROWS        25u              /* Text mode row count              */
#define VGA_BUF_VA           0x10000000u      /* User mapping for the text buffer */
#define VGA_FB_PADDR         0x000B8000u      /* Physical VGA text buffer address */
#define VGA_CELL(fg, bg, ch) ((u16)(u8)(ch) | ((u16)(((bg) << 4) | ((fg) & 0x0F)) << 8)) /* Packed cell */
#define VGA_FG_WHITE         0x0Fu            /* Default foreground color         */
#define VGA_BG_BLACK         0x00u            /* Default background color         */

static volatile u16* const vga = (volatile u16*)VGA_BUF_VA;
static u32 row;
static u32 col;

static void putc_vga(char c) {
    if (c == '\n') {
        row++;
        col = 0;
    } else {
        vga[row * VGA_TEXT_COLS + col] = VGA_CELL(VGA_FG_WHITE, VGA_BG_BLACK, c);
        col++;
        if (col >= VGA_TEXT_COLS) {
            col = 0;
            row++;
        }
    }

    if (row >= VGA_TEXT_ROWS)
        row = 0;
}

static void write_vga(const char* buf, u32 len) {
    for (u32 i = 0; i < len; i++)
        putc_vga(buf[i]);
}

int main(void) {
    if (sys_map_pg((void*)VGA_BUF_VA, (phys_addr_t)VGA_FB_PADDR, VM_FLAG_RW | VM_FLAG_USER) != E_OK)
        return 1;
        
    row = 0;
    col = 0;

    write_vga("[userspace] VGA service online\n", 31);

    char log_buf[128];
    u32 off = 0;

    for (;;) {
        i32 n = sys_log_read(log_buf, (u32)sizeof(log_buf), off);
        if (n > 0) {
            write_vga(log_buf, (u32)n);
            off += (u32)n;
            continue;
        }
        sys_thread_yield();
    }
}
