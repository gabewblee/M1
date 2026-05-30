#include "config.h"
#include "dev/console.h"
#include "dev/evga.h"
#include "dev/klog.h"

#define CONSOLES(X)    \
    X(klog, KLOG_FLAG) \
    X(evga, EVGA_FLAG)

#define CONSOLE_DEF(name, flag)     \
    static const console_s name = { \
        .flags = flag,              \
        .putc  = name##_putc,       \
        .puts  = name##_puts,       \
        .write = name##_write,      \
        .clear = name##_clear,      \
        .init  = name##_init        \
    };

CONSOLES(CONSOLE_DEF)
#undef CONSOLE_DEF

static console_s consoles[CONSOLE_MAX_CNT];
static size_t    cnt;

void console_register_dev(const console_s dev) {
    consoles[cnt++] = dev;
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

void console_putc(u32 flags, const char c) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].putc(c);
}

void console_puts(u32 flags, const char* str) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].puts(str);
}

void console_write(u32 flags, const char* buf, size_t len) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].write(buf, len);
}

void console_clear(u32 flags) {
    for (size_t i = 0; i < cnt; i++)
        if (consoles[i].flags & flags)
            consoles[i].clear();
}

void console_init(void) {
    #define CONSOLE_REGISTER_DEV(name, flag) console_register_dev(name);
    CONSOLES(CONSOLE_REGISTER_DEV)
    #undef CONSOLE_REGISTER_DEV
    for (size_t i = 0; i < cnt; i++)
        consoles[i].init();
}