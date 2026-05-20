#include "config.h"
#include "dev/console.h"
#include "dev/evga.h"
#include "dev/klog.h"

/* Expands to built-in consoles; X(name, flag). */
#define CONSOLES(X)    \
    X(klog, KLOG_FLAG) /* Kernel log console */ \
    X(evga, EVGA_FLAG) /* Boot VGA console     */

/* Instantiates a static console_t for one CONSOLES entry. */
#define CONSOLE_DECL(name, flag) \
    static console_t name = {    \
        .flags = flag,           \
        .putc  = name##_putc,    \
        .puts  = name##_puts,    \
        .write = name##_write,   \
        .clear = name##_clear,   \
        .init  = name##_init     \
    };

CONSOLES(CONSOLE_DECL)
#undef CONSOLE_DECL

static console_t consoles[CONSOLE_MAX_CNT];
static size_t    cnt;

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

void console_clear(u32 flags) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].clear();
}

void console_init(void) {
    /* Registers each console from CONSOLES during console_init. */
    #define CONSOLE_REGISTER_DEV(name, flag) console_register_dev(&name);
    CONSOLES(CONSOLE_REGISTER_DEV)
    #undef CONSOLE_REGISTER_DEV
    for (size_t i = 0; i < cnt; i++)
        consoles[i].init();
}

void console_register_dev(console_t* dev) {
    consoles[cnt++] = *dev;
}

void console_unregister_dev(u32 flags) {
    size_t i = 0;
    while (i < cnt) {
        if (consoles[i].flags & flags) {
            consoles[i] = consoles[cnt - 1];
            cnt--;
            continue;
        }

        i++;
    }
}

void console_exec(u32 flags, console_op_t op, const void* arg, size_t len) {
    switch (op) {
        case CONSOLE_OP_PUTC:
            console_putc(flags, *(const char*)arg);
            break;
        case CONSOLE_OP_PUTS:
            console_puts(flags, (const char*)arg);
            break;
        case CONSOLE_OP_WRITE:
            console_write(flags, (const char*)arg, len);
            break;
        case CONSOLE_OP_CLEAR:
            console_clear(flags);
            break;
        case CONSOLE_OP_INIT:
            console_init();
            break;
    }
}