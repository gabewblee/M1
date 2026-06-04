#include "arch/x86/idt.h"
#include "config.h"
#include "dev/console.h"
#include "kernel/core/panic.h"

void __noreturn panic(const char *msg, const char *file, u32 line) {
    console_clear(ALL_FLAG);
    console_puts(ALL_FLAG, "Kernel panic: ");
    console_puts(ALL_FLAG, msg);
    console_puts(ALL_FLAG, "\nFile: ");
    console_puts(ALL_FLAG, file);
    console_puts(ALL_FLAG, "\nLine: ");

    __asm__ volatile ("cli");
    for (;;)
        __asm__ volatile ("hlt");
}