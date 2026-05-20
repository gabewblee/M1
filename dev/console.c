#include "config.h"
#include "dev/console.h"
#include "dev/klog.h"
#include "dev/vga.h"

static console_t consoles[CONSOLE_MAX_CNT];
static size_t    cnt;
static console_t klog = {
    .flags = KLOG_FLAG,
    .putc  = klog_putc,
    .puts  = klog_puts,
    .write = klog_write,
    .clear = klog_clear,
    .init  = klog_init,
};

static console_t vga = {
    .flags = VGA_FLAG,
    .putc  = vga_putc,
    .puts  = vga_puts,
    .write = vga_write,
    .clear = vga_clear,
    .init  = vga_init,
};

void console_putc(u32 flags, char c) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].putc(c);
}

void console_puts(u32 flags, const char* str) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].puts(str);
}

void console_write(u32 flags, const char* str, size_t len) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].write(str, len);
}

void console_print_decimal(u32 flags, u32 num) {
    char buffer[11];
    int  i = 10;

    buffer[i--] = '\0';

    if (num == 0) {
        console_putc(flags, '0');
        return;
    }

    while (num > 0) {
        u32 q = num / 10;
        u32 r = num - (q * 10);
        buffer[i--] = '0' + r;
        num = q;
    }

    console_puts(flags, &buffer[i + 1]);
}

void console_clear(u32 flags) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].clear();
}

void console_init(void) {
    console_register_dev(&klog);
    console_register_dev(&vga);

    for (size_t i = 0; i < cnt; i++)
        consoles[i].init();
}

void console_register_dev(console_t* dev) {
    consoles[cnt++] = *dev;
}